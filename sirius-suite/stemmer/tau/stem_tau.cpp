
/* This is the Porter stemming algorithm, coded up in ANSI C by the
   author. It may be be regarded as canonical, in that it follows the
   algorithm presented in

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

   Release 1: was many years ago
   Release 2: 11 Apr 2013
   fixes a bug noted by Matt Patenaude <matt@mattpatenaude.com>,

   case 'o': if (ends("\03" "ion") && (b[j] == 's' || b[j] == 't')) break;
   ==>
   case 'o': if (ends("\03" "ion") && j >= k0 && (b[j] == 's' || b[j] == 't')) break;

   to avoid accessing b[k0-1] when the word in b is "ion".
   Release 3: 25 Mar 2014
   fixes a similar bug noted by Klemens Baum <klemensbaum@gmail.com>,
   that if step1ab leaves a one letter result (ied -> i, aing -> a etc),
   step2 and step4 access the byte before the first letter. So we skip
   steps after step1ab unless k > k0.
   */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>      /* for malloc, free */
#include <ctype.h>       /* for isupper, islower, tolower */
#include <time.h>
#include <sys/time.h>
#include <regex.h>
#include "simt.h"

#define INC 50           /* size units in which s is increased */
static int i_max = INC;  /* maximum offset in s */

#define TRUE 1
#define FALSE 0

/* The main part of the stemming algorithm starts here. b is a buffer
   holding a word to be stemmed. The letters are in b[k0], b[k0+1] ...
   ending at b[k]. In fact k0 = 0 in this demo program. k is readjusted
   downwards as the stemming progresses. Zero termination is not in fact
   used in the algorithm.

   Note that only lower case sequences are stemmed. Forcing to lower case
   should be done before stem(...) is called.
   */

// static char * b;       /* buffer for word to be stemmed */
// static int k,k0,j;     /* j is a general offset into the string */
/* cons(i) is TRUE <=> b[i] is a consonant. */

    static int cons(char * b, int i, int k0)
{  switch (b[i])
    {  case 'a': case 'e': case 'i': case 'o': case 'u': return FALSE;
        case 'y': return (i==k0) ? TRUE : !cons(b, i-1, k0);
        default: return TRUE;
    }
}

/* m() measures the number of consonant sequences between k0 and j. if c is
   a consonant sequence and v a vowel sequence, and <..> indicates arbitrary
   presence,

   <c><v>       gives 0
   <c>vc<v>     gives 1
   <c>vcvc<v>   gives 2
   <c>vcvcvc<v> gives 3
   ....
   */

static int m(char * b, int k0, int j)
{  int n = 0;
    int i = k0;
    while(TRUE)
    {  if (i > j) return n;
        if (! cons(b, i, k0)) break; i++;
    }
    i++;
    while(TRUE)
    {  while(TRUE)
        {  if (i > j) return n;
            if (cons(b, i, k0)) break;
            i++;
        }
        i++;
        n++;
        while(TRUE)
        {  if (i > j) return n;
            if (! cons(b, i, k0)) break;
            i++;
        }
        i++;
    }
}

/* vowelinstem() is TRUE <=> k0,...j contains a vowel */

static int vowelinstem(char * b, int j, int k0)
{  
    for (int i = k0; i <= j; i++) if (!cons(b, i, k0)) return TRUE;
    return FALSE;
}

/* doublec(j) is TRUE <=> j,(j-1) contain a double consonant. */

static int doublec(char * b, int j, int k0)
{  if (j < k0+1) return FALSE;
    if (b[j] != b[j-1]) return FALSE;
    return cons(b, j, k0);
}

/* cvc(i) is TRUE <=> i-2,i-1,i has the form consonant - vowel - consonant
   and also if the second c is not w,x or y. this is used when trying to
   restore an e at the end of a short word. e.g.

   cav(e), lov(e), hop(e), crim(e), but
   snow, box, tray.

*/

static int cvc(char * b, int i, int k0)
{  if (i < k0+2 || !cons(b, i, k0) || cons(b, i-1, k0) || !cons(b, i-2, k0)) return FALSE;
    {  int ch = b[i];
        if (ch == 'w' || ch == 'x' || ch == 'y') return FALSE;
    }
    return TRUE;
}

/* ends(s) is TRUE <=> k0,...k ends with the string s. */

static int ends(char * s, char * b, int k, int k0, int &j)
{  
    int length = s[0];
    if (s[length] != b[k]) return FALSE; /* tiny speed-up */
    if (length > k-k0+1) return FALSE;
    if (memcmp(b+k-length+1,s+1,length) != 0) return FALSE;
    j = k-length;
    return TRUE;
}

/* setto(s) sets (j+1),...k to the characters in the string s, readjusting
   k. */

static void setto(char * s, char * b, int &k, int j)
{  
    int length = s[0];
    memmove(b+j+1,s+1,length);
    k = j+length;
}

/* r(s) is used further down. */

static void r(char * s, char * b, int &k, int k0, int j) { if (m(b, k0, j) > 0) setto(s, b, k, j); }

/* step1ab() gets rid of plurals and -ed or -ing. e.g.

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

static void step1ab(char * b, int &k, int k0, int &j)
{
    if (b[k] == 's')
    {  
        if (ends("\04" "sses", b, k, k0, j)) 
            k -= 2;
        else
            if (ends("\03" "ies", b, k, k0, j)) setto("\01" "i", b, k, j); else
                if (b[k-1] != 's') k--;
    }
    if (ends("\03" "eed", b, k, k0, j)) 
    {
        if (m(b, k0, j) > 0)
            k--;
    }
    else
        if ((ends("\02" "ed", b, k, k0, j) || ends("\03" "ing", b, k, k0, j)) && vowelinstem(b, j, k0))
        {  k = j;
            if (ends("\02" "at", b, k, k0, j)) setto("\03" "ate", b, k, j); else
                if (ends("\02" "bl", b, k, k0, j)) setto("\03" "ble", b, k, j); else
                    if (ends("\02" "iz", b, k, k0, j)) setto("\03" "ize", b, k, j); else
                        if (doublec(b, k, k0))
                        {  k--;
                            {  int ch = b[k];
                                if (ch == 'l' || ch == 's' || ch == 'z') k++;
                            }
                        }
                        else if (m(b, k0, j) == 1 && cvc(b, k, k0)) setto("\01" "e", b, k, j);
        }
}

/* step1c() turns terminal y to i when there is another vowel in the stem. */

static void step1c(char * b, int &k, int k0, int &j)
{
    if (ends("\01" "y", b, k, k0, j) && vowelinstem(b, j, k0)) b[k] = 'i'; 
}


/* step2() maps double suffices to single ones. so -ization ( = -ize plus
   -ation) maps to -ize etc. note that the string before the suffix must give
   m() > 0. */

    static void step2(char * b, int &k, int k0, int &j)
{ switch (b[k-1])
    {
        case 'a': if (ends("\07" "ational", b, k, k0, j)) { r("\03" "ate", b, k, k0, j); break; }
                      if (ends("\06" "tional", b, k, k0, j)) { r("\04" "tion", b, k, k0, j); break; }
                  break;
        case 'c': if (ends("\04" "enci", b, k, k0, j)) { r("\04" "ence", b, k, k0, j); break; }
                      if (ends("\04" "anci", b, k, k0, j)) { r("\04" "ance", b, k, k0, j); break; }
                  break;
        case 'e': if (ends("\04" "izer", b, k, k0, j)) { r("\03" "ize", b, k, k0, j); break; }
                      break;
        case 'l': if (ends("\03" "bli", b, k, k0, j)) { r("\03" "ble", b, k, k0, j); break; } /*-DEPARTURE-*/

                      /* To match the published algorithm, replace this line with
                         case 'l': if (ends("\04" "abli")) { r("\04" "able"); break; } */

                      if (ends("\04" "alli", b, k, k0, j)) { r("\02" "al", b, k, k0, j); break; }
                  if (ends("\05" "entli", b, k, k0, j)) { r("\03" "ent", b, k, k0, j); break; }
                  if (ends("\03" "eli", b, k, k0, j)) { r("\01" "e", b, k, k0, j); break; }
                  if (ends("\05" "ousli", b, k, k0, j)) { r("\03" "ous", b, k, k0, j); break; }
                  break;
        case 'o': if (ends("\07" "ization", b, k, k0, j)) { r("\03" "ize", b, k, k0, j); break; }
                      if (ends("\05" "ation", b, k, k0, j)) { r("\03" "ate", b, k, k0, j); break; }
                  if (ends("\04" "ator", b, k, k0, j)) { r("\03" "ate", b, k, k0, j); break; }
                  break;
        case 's': if (ends("\05" "alism", b, k, k0, j)) { r("\02" "al", b, k, k0, j); break; }
                      if (ends("\07" "iveness", b, k, k0, j)) { r("\03" "ive", b, k, k0, j); break; }
                  if (ends("\07" "fulness", b, k, k0, j)) { r("\03" "ful", b, k, k0, j); break; }
                  if (ends("\07" "ousness", b, k, k0, j)) { r("\03" "ous", b, k, k0, j); break; }
                  break;
        case 't': if (ends("\05" "aliti", b, k, k0, j)) { r("\02" "al", b, k, k0, j); break; }
                      if (ends("\05" "iviti", b, k, k0, j)) { r("\03" "ive", b, k, k0, j); break; }
                  if (ends("\06" "biliti", b, k, k0, j)) { r("\03" "ble", b, k, k0, j); break; }
                  break;
        case 'g': if (ends("\04" "logi", b, k, k0, j)) { r("\03" "log", b, k, k0, j); break; } /*-DEPARTURE-*/

                      /* To match the published algorithm, delete this line */

    } }

/* step3() deals with -ic-, -full, -ness etc. similar strategy to step2. */

static void step3(char * b, int &k, int k0, int &j)
{
    switch (b[k])
    {
        case 'e': if (ends("\05" "icate", b, k, k0, j)) { r("\02" "ic", b, k, k0, j); break; }
                      if (ends("\05" "ative", b, k, k0, j)) { r("\00" "", b, k, k0, j); break; }
                  if (ends("\05" "alize", b, k, k0, j)) { r("\02" "al", b, k, k0, j); break; }
                  break;
        case 'i': if (ends("\05" "iciti", b, k, k0, j)) { r("\02" "ic", b, k, k0, j); break; }
                      break;
        case 'l': if (ends("\04" "ical", b, k, k0, j)) { r("\02" "ic", b, k, k0, j); break; }
                      if (ends("\03" "ful", b, k, k0, j)) { r("\00" "", b, k, k0, j); break; }
                  break;
        case 's': if (ends("\04" "ness", b, k, k0, j)) { r("\00" "", b, k, k0, j); break; }
                      break;
    }
}

/* step4() takes off -ant, -ence etc., in context <c>vcvc<v>. */

static void step4(char * b, int &k, int k0, int &j)
{  
    switch (b[k-1])
    {  
        case 'a': if (ends("\02" "al", b, k, k0, j)) break; return;
        case 'c': if (ends("\04" "ance", b, k, k0, j)) break;
                      if (ends("\04" "ence", b, k, k0, j)) break; return;
        case 'e': if (ends("\02" "er", b, k, k0, j)) break; return;
        case 'i': if (ends("\02" "ic", b, k, k0, j)) break; return;
        case 'l': if (ends("\04" "able", b, k, k0, j)) break;
                      if (ends("\04" "ible", b, k, k0, j)) break; return;
        case 'n': if (ends("\03" "ant", b, k, k0, j)) break;
                      if (ends("\05" "ement", b, k, k0, j)) break;
                  if (ends("\04" "ment", b, k, k0, j)) break;
                  if (ends("\03" "ent", b, k, k0, j)) break; return;
        case 'o': if (ends("\03" "ion", b, k, k0, j) && j >= k0 && (b[j] == 's' || b[j] == 't')) break;
                      if (ends("\02" "ou", b, k, k0, j)) break; return;
                  /* takes care of -ous */
        case 's': if (ends("\03" "ism", b, k, k0, j)) break; return;
        case 't': if (ends("\03" "ate", b, k, k0, j)) break;
                      if (ends("\03" "iti", b, k, k0, j)) break; return;
        case 'u': if (ends("\03" "ous", b, k, k0, j)) break; return;
        case 'v': if (ends("\03" "ive", b, k, k0, j)) break; return;
        case 'z': if (ends("\03" "ize", b, k, k0, j)) break; return;
        default: return;
    }
    if (m(b, k0, j) > 1) k = j;
}

/* step5() removes a final -e if m() > 1, and changes -ll to -l if
   m() > 1. */

static void step5(char * b, int &k, int k0, int &j)
{  j = k;
    if (b[k] == 'e')
    {  int a = m(b, k0, j);
        if (a > 1 || a == 1 && !cvc(b, k-1, k0)) k--;
    }
    if (b[k] == 'l' && doublec(b, k, k0) && m(b, k0, j) > 1) k--;
}

/* In stem(p,i*4,j), p is a char pointer, and the string to be stemmed is from
   p[i] to p[j] inclusive. Typically i is zero and j is the offset to the last
   character of a string, (p[j+1] == '\0'). The stemmer adjusts the
   characters p[i] ... p[j] and returns the new end-point of the string, k.
   Stemming never increases word length, so i <= k <= j. To turn the stemmer
   into a module, declare 'stem' as extern, and delete the remainder of this
   file.
   */

int stem(char * b, int i, int j)
{  
    int k  = j;
    int k0 = i; /* copy the parameters into statics */
    if (k <= k0+1)
        return k; /*-DEPARTURE-*/

    /* With this line, strings of length 1 or 2 don't go through the
       stemming process, although no mention is made of this in the
       published algorithm. Remove the line to match the published
       algorithm. */

    step1ab(b, k, k0, j);
    if (k > k0)
    {
        step1c(b, k, k0, j);
        step2(b, k, k0, j);
        step3(b, k, k0, j);
        step4(b, k, k0, j);
        step5(b, k, k0, j);
    }
    return k;
}

/*--------------------stemmer definition ends here------------------------*/

#define LETTER(ch) (isupper(ch) || islower(ch))

static void stemfile(char **words, int nWords)
{
    int p = 0;
    while(p < nWords)
    {
        int letPos = 0;
        char ch = words[p][letPos];
        while(ch != '\0')
        {
            ++letPos;
            ch = words[p][letPos];
        }
        words[p][stem(words[p], 0, letPos-1)+1] = '\0';
        ++p;
    }
}

static void stemfile_tau(char **words, int nWords)
{
    ar::simt_tau::par_for(nWords, [&, &words](size_t p)
    {
        int letPos = 0;
        char ch = words[p][letPos];
        while(ch != '\0')
        {
        ++letPos;
        ch = words[p][letPos];
        }
        words[p][stem(words[p], 0, letPos-1)+1] = '\0';
    });
}

int main(int argc, char * argv[])
{
    /* Timing */
    struct timeval tv1, tv2;
    unsigned int runtime = 0;
    unsigned int totalruntimetau = 0;
    int nWords = 256;
    int max_word_size = 100;
    FILE * f;
    for (int i = 1; i < argc; i++)
    {  
        f = fopen(argv[i],"r");
        if (f == 0)
        {
            printf("File %s not found\n", argv[i]);
            exit(1);
        }
    }

    char ** buffer = (char **) malloc(sizeof(char*) * nWords);

    if(buffer == NULL)
    {
        fputs ("Memory error",stderr);
        exit (2);
    }

    // read words into buffer
    int i = 0;
    for (i = 0; ; ++i)
    {
        int j;
        if (i >= nWords) //increase size of buffer
        {
            int tempWords = 2*nWords;
            buffer = (char **)realloc(buffer, sizeof(char*) * tempWords);
            nWords = tempWords;
        }
        buffer[i]     = (char*) malloc(max_word_size);
        if (fgets(buffer[i], max_word_size-1, f) == NULL)
            break;

        for (j = strlen(buffer[i])-1; j>=0 && (buffer[i][j]=='\n' || buffer[i][j]=='\r'); --j)
            ;
        buffer[i][j+1]='\0';
    }

    // JAH:
    // Uncomment to read a specific number of words into a tau buffer
    // FILE * f1;
    // for (int i = 1; i < argc; i++)
    // {  
    //    f1 = fopen(argv[i],"r");
    //    if (f1 == 0)
    //    {
    //      printf("File %s not found\n", argv[i]);
    //      exit(1);
    //    }
    // }
    // char ** buffer_tau = (char **) malloc(sizeof(char*) * nWords);
    //
    // for(int i = 0; i < nWords; ++i)
    //     buffer_tau[i] = (char*) malloc(max_word_size);
    //
    // for (i = 0; i < nWords; ++i)
    // {
    //     int j;
    //     if (fgets(buffer_tau[i], max_word_size-1, f) == NULL)
    //         break;
    //
    //     for (j = strlen(buffer_tau[i])-1; j>=0 && (buffer_tau[i][j]=='\n' || buffer_tau[i][j]=='\r'); --j)
    //         ;
    //     buffer_tau[i][j+1]='\0';
    // }

    // set final nWords read
    nWords = i;

    gettimeofday(&tv1,NULL);
    stemfile_tau(buffer, nWords);
    gettimeofday(&tv2,NULL);
    totalruntimetau = (tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec);
    // for(int i = 0; i < nWords; ++i)
    //   printf("TAU: %s\n",buffer[i]);

    printf("TAU runtime: %4.2f ms\n", (double)totalruntimetau/1000);

    for(int i = 0; i < nWords; ++i) {
        free(buffer[i]);
        // free(buffer_tau[i]);
    }

    free(buffer);
    // free(buffer_tau);

    return 0;
}

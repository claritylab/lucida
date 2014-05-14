#ifndef DRUTILS_H
#define DRUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_IGNORE -1

/************************************* Errors ********************************/
extern void beep(void);
extern void debug(char *fmt, ...);
extern void error(char *fmt, ...);
extern void fatalError(char *fmt, ...);
extern int  sys(char *fmt, ...);

/************************************* Memory ********************************/
extern long long Memory, MaxMemory;

extern void smartAllocInit(int numTypes, int hashSize);
extern void smartAllocRegister(int type, char *name);
extern void smartAllocReport(void);
extern void *smartMalloc(unsigned size, int type);
extern void *smartCalloc(unsigned num, unsigned size, int type);
extern void *smartRealloc(void *ptr, unsigned size, int type);
extern void smartFree(void *ptr, int type);
/*
extern void *safeMalloc(unsigned size, char *name);
extern void *safeCalloc(unsigned num, unsigned size, char *name);
extern void *safeRealloc(void *ptr, unsigned size, char *name);
*/
extern int  *intArray(int n, int memType);
extern int  **intMatrix(int m, int n, int memType);
extern real *realArray(int n, int memType);
extern real **realMatrix(int m, int n, int memType);
extern char *charArray(int m, int memType);
extern char **charMatrix(int m, int n, int memType);
extern void **pointerArray(int n, int memType);
extern void ***pointerMatrix(int m, int n, int memType);

#define FREE(p, type) {if (p) {smartFree(p, type); p = NULL;}}
#define FREE_MATRIX(p, type) {if (p) {if (p[0]) smartFree(p[0], type); smartFree(p, type); p = NULL;}}

/************************************* Files *********************************/
extern FILE *fatalReadFile(char *filename);
extern FILE *fatalWriteFile(char *filename, flag append);
extern FILE *readFile(char *fileName);
extern FILE *writeFile(char *fileName, flag append);
extern void closeFile(FILE *file);

/************************************ Strings ********************************/
typedef struct string {
  int memType;
  int maxChars;
  int numChars; /* Equals strlen s */
  char *s;
} *String;

extern char *copyString(char *s, int memType);
extern flag isBlank(char *string);
extern String newString(int maxChars, int memType);
extern String newStringCopy(char *s, int memType);
extern void stringSize(String S, int maxChars);
extern void stringCat(String S, char c);
extern void stringSet(String S, int idx, char c);
extern void stringAppend(String S, char *t);
extern void clearString(String S);
extern void freeString(String S);
extern flag readLine(String S, FILE *file);
extern void readFileIntoString(String S, char *fileName);

extern int  smartCompareStrings(char *A, char *B);

/*************************** Variable Size Pointer Arrays ********************/
typedef struct parray {
  int memType;
  int maxItems;
  int numItems;
  void **item;
} *Parray;

extern Parray newParray(int maxItems, int memType);
extern void parraySize(Parray P, int maxItems);
extern void parrayCat(Parray P, void *p);
extern void parraySet(Parray P, int idx, void *p);
extern void shrinkParray(Parray P);
extern void clearParray(Parray P);
extern void freeParray(Parray P);

/*************************** Variable Size Data Arrays ***********************/

typedef struct darray {
  int memType;
  int itemSize;
  int maxItems;
  int numItems;
  char *item;
} *Darray;

extern Darray newDarray(int itemSize, int maxItems, int memType);
extern void darraySize(Darray D, int maxItems);
extern void darrayCat(Darray D, char *p);
extern void darraySet(Darray D, int idx, char *p);
extern void shrinkDarray(Darray D);
extern void clearDarray(Darray D);
extern void freeDarray(Darray D);

/********************************** Randomness *******************************/
extern void seedRand(unsigned int seed);
extern void timeSeedRand(void);
extern unsigned int getSeed(void);
extern int  randInt(int max);
extern real randReal(real mean, real range);
extern real randProb(void);
extern real randGaussian(real mean, real range);
extern void randSort(int *array, int n);

/************************************* Time **********************************/
extern unsigned long timeElapsed(unsigned long a, unsigned long b);
extern unsigned long getTime(void);
extern unsigned long getUTime(void);
extern void formatTime(unsigned long time, char *dest);
extern flag printProgress(unsigned int i, unsigned int max);

typedef struct loopTimer {
  /* These are counters: */
  unsigned long lasti;
  /* These are in msec: */
  unsigned long start;
  unsigned long last;
  real mean;
  /* These are in sec: */
  unsigned long elapsed;
  unsigned long left;
  unsigned long total;
} *LoopTimer;

extern LoopTimer newLoopTimer(int memType);
extern void updateLoopTimer(LoopTimer T, int i, int total);

/************************************* Math **********************************/
extern int  imin(int a, int b);
extern int  imax(int a, int b);
extern real rmin(real a, real b);
extern real rmax(real a, real b);

#ifndef MIN
#  define MIN(a,b)    (((a)<(b))?(a):(b))
#endif /* MIN */
#ifndef MAX
#  define MAX(a,b)    (((a)>(b))?(a):(b))
#endif /* MAX */
#ifndef SQUARE
#  define SQUARE(x)   ((x) * (x))
#endif /* SQUARE */

/*********************************** Binary IO *******************************/
extern flag readBinChar(FILE *file, char *val);     /* returns error code */
extern flag readBinShort(FILE *file, short *val);   /* returns error code */
extern flag readBinInt(FILE *file, int *val);       /* returns error code */
extern flag readBinReal(FILE *file, real *val);     /* returns error code */
extern flag readBinDouble(FILE *file, double *val); /* returns error code */
extern flag readBinFlag(FILE *file, flag *val);     /* returns error code */
extern flag readBinString(FILE *file, String S);    /* returns error code */
extern flag writeBinChar(FILE *file, char c);
extern flag writeBinShort(FILE *file, short x);
extern flag writeBinInt(FILE *file, int x);
extern flag writeBinReal(FILE *file, real r);
extern flag writeBinDouble(FILE *file, double r);
extern flag writeBinFlag(FILE *file, flag f);
extern void writeBinString(FILE *file, char *s, int len);
extern void writeReal(FILE *file, real r, char *pre, char *post);

#ifdef __cplusplus
}
#endif

#endif /* DRUTILS_H */

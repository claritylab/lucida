#include "drutils.h"
#include "system.h"
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "hash.h"

union nanfunion NaNfunion = NaNfbytes;
union nandunion NaNdunion = NaNdbytes;

/************************************* Errors ********************************/

void beep(void) {
  fputc('\a', stderr);
  fflush(stderr);
}

void debug(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fflush(stderr);
}

void error(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  beep();
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}

void fatalError(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  beep();
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\a\n");
  va_end(args);
  exit(1);
}

int sys(char *fmt, ...) {
  char command[256];
  va_list args;
  va_start(args, fmt);
  vsprintf(command, fmt, args);
  va_end(args);
  return system(command);
}


/************************************* Memory ********************************/

int  MemTypes = 0;
char **MemTypeName;

#ifdef SMART_ALLOC
typedef struct allrec {
  void *ptr;
  unsigned int size;
  int type;
} *AllRec;

long long Pointers = 0, MaxPointers = 0;
int  *PointersTotal, *PointersNow;
long long Memory = 0, MaxMemory = 0;
long long *MemTotal, *MemNow;

HashTable AllocHash;

static unsigned long hashPointer(void *p, unsigned long n) {
  return ((unsigned long) p) % n;
}

static int comparePointers(void *a, void *b) {
  return (a == b) ? 0 : (a > b) ? 1 : -1;
}

static AllRec lookupPtr(void *ptr) {
  return (AllRec) hashTableLookup(AllocHash, ptr);
}

static void registerPtr(void *ptr, unsigned int size, int type) {
  AllRec R;
  if (type < 0) return;
  if (type >= MemTypes)
    fatalError("smartAlloc: type %d out of range", type);

  if (lookupPtr(ptr)) 
    fatalError("pointer %p has already been registered", ptr);
  R = (AllRec) malloc(sizeof *R);
  R->ptr  = ptr;
  R->size = size;
  R->type = type;

  PointersTotal[type]++;
  PointersNow[type]++;
  if (++Pointers > MaxPointers) MaxPointers = Pointers;

  MemTotal[type] += size;
  MemNow[type] += size;
  hashTableInsert(AllocHash, ptr, R, FALSE);

  Memory += size;
  if (Memory > MaxMemory) MaxMemory = Memory;
}

static void updatePtr(void *old, void *new, unsigned int newSize, int type) {
  AllRec R = lookupPtr(old);
  if (type < 0) return;
  if (type >= MemTypes)
    fatalError("smartAlloc: type %d out of range", type);

  if (!R) 
    fatalError("attempted to update pointer %p that doesn't exist", old);
  if (R->type != type)
    fatalError("attempted to update pointer %p with type %s instead of %s", 
	       old, MemTypeName[type], MemTypeName[R->type]);
  MemTotal[R->type] -= R->size;
  MemNow[R->type]   -= R->size;
  Memory -= R->size;
  hashTableRemove(AllocHash, old);
  R->size = newSize;
  R->ptr = new;
  MemTotal[R->type] += R->size;
  MemNow[R->type]   += R->size;
  Memory += R->size;
  if (Memory > MaxMemory) MaxMemory = Memory;
  hashTableInsert(AllocHash, new, R, FALSE);
}

static void freePtr(void *ptr, int type) {
  AllRec R = lookupPtr(ptr);
  if (!R) {
    if (type < 0) return;
    else fatalError("attempted to free pointer %p that doesn't exist", ptr);
  }
  if (R->type != type)
    fatalError("attempted to free pointer %p with type %s instead of %s", 
	       ptr, MemTypeName[type], MemTypeName[R->type]);
  PointersNow[R->type]--;
  Pointers--;
  MemNow[R->type] -= R->size;
  Memory -= R->size;
  hashTableRemove(AllocHash, ptr);
  free(R);
}
#endif

void smartAllocInit(int numTypes, int hashSize) {
  MemTypes = numTypes;
  MemTypeName   = (char **) calloc(MemTypes, sizeof(char *));
#ifdef SMART_ALLOC
  PointersTotal = (int *)  calloc(MemTypes, sizeof(int));
  PointersNow   = (int *)  calloc(MemTypes, sizeof(int));
  MemTotal      = (long long *) calloc(MemTypes, sizeof(long long));
  MemNow        = (long long *) calloc(MemTypes, sizeof(long long));
  AllocHash = hashTableCreate(hashSize, 2.0, hashPointer, comparePointers,
			      MEM_IGNORE);
#endif
}

void smartAllocRegister(int type, char *name) {
  if (type < 0 || type >= MemTypes) 
    fatalError("smartAllocRegister: type %d out of range", type);
  if (MemTypeName[type])
    fatalError("smartAllocRegister: type %d already given name %s", 
	       type, MemTypeName[type]);
  MemTypeName[type] = name;
}

void smartAllocReport(void) {
#ifdef SMART_ALLOC
  int i;
  unsigned long long pointersTotal = 0, pointersNow = 0;
  unsigned long long memTotal = 0, memNow = 0;
  for (i = 0; i < MemTypes; i++) {
    pointersTotal += PointersTotal[i];
    pointersNow   += PointersNow[i];
    memTotal      += MemTotal[i];
    memNow        += MemNow[i];
  }
  printf("--------------------------------------------------------------------------------\n");
  printf("Total Pointers: %10lld    Total Memory:   %10lld\n", 
	 pointersTotal, memTotal);
  printf("Max Pointers:   %10lld    Max Memory:     %10lld\n",
	 MaxPointers, MaxMemory);
  printf("Pointers Now:   %10lld    Memory Now:     %10lld\n\n", 
	 pointersNow, memNow);

  if (!pointersTotal) pointersTotal = 1;
  if (!pointersNow)   pointersNow = 1;
  if (!memTotal)      memTotal = 1;
  if (!memNow)        memNow = 1;

  printf("Pointer Type__Total Pointers____Pointers Now____Total Memory_______Memory Now__\n");
  for (i = 0; i < MemTypes; i++) {
    printf("%-12s %8d %5.2f%% %8d %5.2f%% %9lld %5.2f%% %9lld %5.2f%%\n", 
	   MemTypeName[i], 
	   PointersTotal[i], (double) PointersTotal[i] * 100 / pointersTotal,
	   PointersNow[i], (double) PointersNow[i] * 100 / pointersNow,
	   MemTotal[i], (double) MemTotal[i] * 100 / memTotal,
	   MemNow[i], (double) MemNow[i] * 100 / memNow);
  }
  printf("--------------------------------------------------------------------------------\n");
#endif
}

void *smartMalloc(unsigned size, int type) {
  void *ptr;
  if (size == 0) return NULL;
  ptr = malloc(size);
  if (!ptr) fatalError("failed to malloc %u bytes of type %s", 
		       size, MemTypeName[type]);
#ifdef SMART_ALLOC
  registerPtr(ptr, size, type);
#endif
  return ptr;
}

void *smartCalloc(unsigned num, unsigned size, int type) {
  void *ptr;
  if (num == 0 || size == 0) return NULL;
  ptr = calloc(num, size);
  if (!ptr) fatalError("failed to calloc %u bytes of type %s", 
		       num * size, MemTypeName[type]);
#ifdef SMART_ALLOC
  registerPtr(ptr, num * size, type);
#endif
  return ptr;
}

void *smartRealloc(void *ptr, unsigned size, int type) {
  void *new;
  if (!ptr) return smartMalloc(size, type);
  new = realloc(ptr, size);
  if (!new) fatalError("failed to realloc %p to %u bytes of type %s", 
		       ptr, size, MemTypeName[type]);
#ifdef SMART_ALLOC
  updatePtr(ptr, new, size, type);
#endif  
  return new;
}

void smartFree(void *ptr, int type) {
  if (!ptr) return;
#ifdef SMART_ALLOC
  freePtr(ptr, type);
#endif
  free(ptr);
}

#ifdef JUNK
/* Returning NULL is useful when someone allocates an array of size 0 */
void *safeMalloc(unsigned size, char *name) {
  void *new;
  if (size == 0) return NULL;
  new = malloc(size);
  if (!new) fatalError("failed to allocate %s of size %d", name, size);
  return new;
}

void *safeCalloc(unsigned num, unsigned size, char *name) {
  void *new;
  if (size == 0 || num == 0) return NULL;
  new = calloc(num, size);
  if (!new) fatalError("failed to allocate %s of size %dx%d", name, num, size);
  return new;
}

void *safeRealloc(void *ptr, unsigned size, char *name) {
  void *new;
  if (size == 0) return NULL;
  new = realloc(ptr, size);
  if (!new) fatalError("failed to reallocate %s to size %d", name, size);
  return new;
}
#endif

int *intArray(int n, int memType) {
  return (int *) smartCalloc(n, sizeof(int), memType);
}

int **intMatrix(int m, int n, int memType) {
  int i;
  int **M = (int **) smartMalloc(m * sizeof(int *), memType);
  M[0] = (int *) smartCalloc(m * n, sizeof(int), memType);
  for (i = 1; i < m; i++)
    M[i] = M[i - 1] + n;
  return M;
}

real *realArray(int n, int memType) {
  return (real *) smartCalloc(n, sizeof(real), memType);
}

real **realMatrix(int m, int n, int memType) {
  int i;
  real **M = (real **) smartMalloc(m * sizeof(real *), memType);
  M[0] = (real *) smartCalloc(m * n, sizeof(real), memType);
  for (i = 1; i < m; i++)
    M[i] = M[i - 1] + n;
  return M;
}

char *charArray(int n, int memType) {
  return (char *) smartCalloc(n, sizeof(char), memType);
}

char **charMatrix(int m, int n, int memType) {
  int i;
  char **M = (char **) smartMalloc(m * sizeof(char *), memType);
  M[0] = (char *) smartCalloc(m * n, sizeof(char), memType);
  for (i = 1; i < m; i++)
    M[i] = M[i - 1] + n;
  return M;
}

void **pointerArray(int n, int memType) {
  return (void **) smartCalloc(n, sizeof(void *), memType);
}

void ***pointerMatrix(int m, int n, int memType) {
  int i;
  void ***M = (void ***) smartMalloc(m * sizeof(void **), memType);
  M[0] = (void **) smartCalloc(m * n, sizeof(void *), memType);
  for (i = 1; i < m; i++)
    M[i] = M[i - 1] + n;
  return M;
}

/************************************* Files *********************************/

FILE *Pipe[MAX_PIPES];
int numPipes = 0;

static void registerPipe(FILE *p) {
  if (numPipes >= MAX_PIPES) error("Too many pipes open");
  Pipe[numPipes++] = p;
}

static flag isPipe(FILE *p) {
  int i;
  for (i = 0; i < numPipes && Pipe[i] != p; i++);
  if (i == numPipes) return FALSE;
  Pipe[i] = Pipe[--numPipes];
  return TRUE;
}

static int stringEndsIn(char *s, char *t) {
  int ls = strlen(s);
  int lt = strlen(t);
  if (ls < lt) return FALSE;
  return (strcmp(s + ls - lt, t)) ? FALSE : TRUE;
}

static FILE *openPipe(char *pipeName, char *mode) {
  FILE *pipe;
  fflush(stdout);
  if ((pipe = popen(pipeName, mode))) registerPipe(pipe);
  return pipe;
}

static FILE *readZippedFile(char *command, char *fileName) {
  char buf[MAX_FILENAME];
  sprintf(buf, "%s < %s 2>/dev/null", command, fileName);
  return openPipe(buf, "r");
}

FILE *fatalReadFile(char *filename) {
  FILE *file;
  if (!(file = readFile(filename)))
    fatalError("couldn't read the file %s", filename);
  return file;
}

FILE *fatalWriteFile(char *filename, flag append) {
  FILE *file;
  if (!(file = writeFile(filename, append)))
    fatalError("couldn't write the file %s", filename);
  return file;
}

/* Will silently return NULL if file couldn't be opened */
FILE *readFile(char *fileName) {
  char fileBuf[MAX_FILENAME];
  struct stat statbuf;

  /* Special file name */
  if (!strcmp(fileName, "-"))
    return stdin;
  
  /* If it is a pipe */
  if (fileName[0] == '|')
    return openPipe(fileName + 1, "r");

  /* Check if already ends in .gz or .Z and assume compressed */
  if (stringEndsIn(fileName, ".gz") || stringEndsIn(fileName, ".Z")) {
    if (!stat(fileName, &statbuf))
      return readZippedFile(UNZIP, fileName);
    return NULL;
  }
  /* Check if already ends in .bz or .bz2 and assume compressed */
  if (stringEndsIn(fileName, ".bz") || stringEndsIn(fileName, ".bz2")) {
    if (!stat(fileName, &statbuf))
      return readZippedFile(BUNZIP2, fileName);
    return NULL;
  }
  /* Try just opening normally */
  if (!stat(fileName, &statbuf))
    return fopen(fileName, "r");
  /* Try adding .gz */
  sprintf(fileBuf, "%s.gz", fileName);
  if (!stat(fileBuf, &statbuf))
    return readZippedFile(UNZIP, fileBuf);
  /* Try adding .Z */
  sprintf(fileBuf, "%s.Z", fileName);
  if (!stat(fileBuf, &statbuf))
    return readZippedFile(UNZIP, fileBuf);
  /* Try adding .bz2 */
  sprintf(fileBuf, "%s.bz2", fileName);
  if (!stat(fileBuf, &statbuf))
    return readZippedFile(BUNZIP2, fileBuf);
  /* Try adding .bz */
  sprintf(fileBuf, "%s.bz", fileName);
  if (!stat(fileBuf, &statbuf))
    return readZippedFile(BUNZIP2, fileBuf);

  return NULL;
}

static FILE *writeZippedFile(char *fileName, flag append) {
  char buf[MAX_FILENAME];
  char *op = (append) ? ">>" : ">";
  if (stringEndsIn(fileName, ".bz2") || stringEndsIn(fileName, ".bz"))
    sprintf(buf, "%s %s \"%s\"", BZIP2, op, fileName);
  else if (stringEndsIn(fileName, ".Z"))
    sprintf(buf, "%s %s \"%s\"", COMPRESS, op, fileName);
  else
    sprintf(buf, "%s %s \"%s\"", ZIP, op, fileName);
  return openPipe(buf, "w");
}

FILE *writeFile(char *fileName, flag append) {
  /* Special file name */
  if (!strcmp(fileName, "-"))
    return stdout;
  
  /* If it is a pipe */
  if (fileName[0] == '|')
    return openPipe(fileName + 1, "w");

  /* Check if ends in .gz, .Z, .bz, .bz2 */
  if (stringEndsIn(fileName, ".gz") || stringEndsIn(fileName, ".Z") ||
      stringEndsIn(fileName, ".bz") || stringEndsIn(fileName, ".bz2"))
    return writeZippedFile(fileName, append);
  return (append) ? fopen(fileName, "a") : fopen(fileName, "w");
}

/* Could be a file or a stream. */
void closeFile(FILE *file) {
  if (file == stdin || file == stdout) return;
  if (isPipe(file)) pclose(file);
  else fclose(file);
}


/************************************ Strings ********************************/

char *copyString(char *string, int memType) {
  int len;
  char *new;
  if (!string) return NULL;
  len = strlen(string) + 1;
  new = (char *) smartMalloc(len, memType);
  memcpy(new, string, len);
  return new;
}

/* 1 if string contains nothing but tabs and spaces */
flag isBlank(char *string) {
  char *s;
  for (s = string; *s; s++)
    if (!isspace((int) s[0])) return FALSE;
  return TRUE;
}

String newString(int maxChars, int memType) {
  String S = (String) smartMalloc(sizeof *S, memType);
  S->memType = memType;
  S->maxChars = maxChars;
  if (maxChars < 1) S->maxChars = 1;
  S->numChars = 0;
  S->s = (char *) smartMalloc(S->maxChars, memType);
  S->s[0] = '\0';
  return S;
}

String newStringCopy(char *s, int memType) {
  int len = strlen(s);
  String S = newString(len + 1, memType);
  strcpy(S->s, s);
  S->numChars = len;
  return S;
}

/* Ensures that the S can hold a string of length maxChars */
void stringSize(String S, int maxChars) {
  if (S->maxChars <= maxChars) {
    S->maxChars = maxChars + 1;
    S->s = (char *) smartRealloc(S->s, S->maxChars, S->memType);
  }
}

void stringCat(String S, char c) {
  if ((S->numChars + 1) >= S->maxChars) {
    S->maxChars *= 2;
    S->s = (char *) smartRealloc(S->s, S->maxChars, S->memType);
  }
  S->s[S->numChars++] = c;
  S->s[S->numChars] = '\0';
}

void stringSet(String S, int idx, char c) {
  if (S->maxChars <= idx) {
    S->maxChars = ((S->maxChars * 2) > (idx + 1)) ? S->maxChars * 2 : idx + 1;
    S->s = (char *) smartRealloc(S->s, S->maxChars, S->memType);
  }
  S->s[idx] = c;
}

void stringAppend(String S, char *t) {
  int tlen = strlen(t);
  if ((S->numChars + tlen + 1) > S->maxChars) {
    S->maxChars = ((S->maxChars * 2) > (S->numChars + tlen + 1)) ? 
      S->maxChars * 2 : S->numChars + tlen + 1;
    S->s = (char *) smartRealloc(S->s, S->maxChars, S->memType);
  }
  memcpy(S->s + S->numChars, t, tlen + 1);
  S->numChars += tlen;
}

void clearString(String S) {
  S->numChars = 0;
  S->s[0] = '\0';
}

void freeString(String S) {
  if (S) {
    smartFree(S->s, S->memType);
    smartFree(S, S->memType);
  }
}

#define LINE_SIZE 1024

flag readLine(String S, FILE *file) {
  char buf[LINE_SIZE];
  clearString(S);
  do {
    if (!fgets(buf, LINE_SIZE, file)) 
      return (S->numChars > 0);
    stringAppend(S, buf);
  } while (S->s[S->numChars - 1] != '\n');
  S->s[--S->numChars] = '\0';
  return TRUE;
}

void readFileIntoString(String S, char *fileName) {
  int bytes;
  FILE *file;
  if (!(file = readFile(fileName))) {
    error("couldn't open file \"%s\"", fileName);
    return;
  }
  while ((bytes = fread(S->s + S->numChars, 1, 
                        S->maxChars - S->numChars, file)) > 0) {
    S->numChars += bytes;
    if ((S->numChars * 2) > S->maxChars)
      S->maxChars *= 2;
    S->s = (char *) smartRealloc(S->s, S->maxChars, S->memType);
  }
  S->s[S->numChars++] = '\0';
  closeFile(file);
}


int getField(char *s, int *val, int *len) {
  if (!*s) {
    *len = 0;
    return 0;
  } if (isdigit(*s)) {
    int v = 0, l = 0;
    for (; *s && isdigit(*s); l++, s++)
      v = 10 * v + *s - '0';
    *val = v;
    *len = l;
    return 1;
  } else {
    int l = 0;
    for (; *s && !isdigit(*s); l++, s++);
    *len = l;
    return 2;
  }
}

/* This breaks strings into integer or text fields and sorts accordingly. */
int smartCompareStrings(char *A, char *B) {
  int valA, valB, lenA, lenB;
  char typeA, typeB;
  while (1) {
    typeA = getField(A, &valA, &lenA);
    typeB = getField(B, &valB, &lenB);
    if (typeA == 0) {
      if (typeB == 0) return 0;
      else return -1;
    } else {
      if (typeB == 0) return 1;
      if (typeA == 1) {
        if (typeB == 2) return -1;
        if (valA < valB) return -1;
        else if (valA > valB) return 1;
      } else {
        int c;
        if (typeB == 1) return 1;
        c = strncmp(A, B, imin(lenA, lenB));
        if (c < 0) return -1;
        else if (c > 0) return 1;
      }
    }
    A += lenA; B += lenB;
  }
}

/*************************** Variable Size Pointer Arrays ********************/

Parray newParray(int maxItems, int memType) {
  Parray P = (Parray) smartMalloc(sizeof *P, memType);
  P->memType = memType;
  if (maxItems < 1) maxItems = 1;
  P->maxItems = maxItems;
  P->item = (void **) smartMalloc(maxItems * sizeof(void *), memType);
  P->numItems = 0;
  return P;
}

/* Ensures that the P can hold maxItems */
void parraySize(Parray P, int maxItems) {
  if (P->maxItems < maxItems) {
    P->maxItems = maxItems;
    P->item = (void **) smartRealloc(P->item, P->maxItems * sizeof(void *), 
				     P->memType);
  }
}

void parrayCat(Parray P, void *p) {
  if (P->numItems >= P->maxItems) {
    P->maxItems *= 2;
    P->item = (void **) smartRealloc(P->item, P->maxItems * sizeof(void *), 
				     P->memType);
  }
  P->item[P->numItems++] = p;
}

void parraySet(Parray P, int idx, void *p) {
  if (P->maxItems <= idx) {
    P->maxItems = ((P->maxItems * 2) > idx) ? P->maxItems * 2 : idx + 1;
    P->item = (void **) smartRealloc(P->item, P->maxItems * sizeof(void *),
				     P->memType);
  }
  P->item[idx] = p;
  if (P->numItems <= idx) P->numItems = idx + 1;
}

void shrinkParray(Parray P) {
  if (P->maxItems > P->numItems) {
    P->maxItems = P->numItems;
    if (P->maxItems < 1) P->maxItems = 1;
    P->item = (void **) smartRealloc(P->item, P->maxItems * sizeof(void *),
				     P->memType);
  }
}

void clearParray(Parray P) {
  P->numItems = 0;
}

void freeParray(Parray P) {
  if (P) {
    smartFree(P->item, P->memType);
    smartFree(P, P->memType);
  }
}


/*************************** Variable Size Data Arrays ***********************/

Darray newDarray(int itemSize, int maxItems, int memType) {
  Darray D = (Darray) smartMalloc(sizeof *D, memType);
  D->itemSize = itemSize;
  D->memType = memType;
  if (maxItems < 1) maxItems = 1;
  D->maxItems = maxItems;
  D->item = (char *) smartMalloc(itemSize * maxItems, memType);
  D->numItems = 0;
  return D;
}

/* Ensures that the D can hold maxItems */
void darraySize(Darray D, int maxItems) {
  if (D->maxItems < maxItems) {
    D->maxItems = maxItems;
    if (D->item)
      D->item = (char *) smartRealloc(D->item, D->itemSize * D->maxItems, 
                                      D->memType);
    else D->item = (char *) smartMalloc(D->itemSize * D->maxItems, D->memType);
  }
}

void darrayCat(Darray D, char *p) {
  if (D->numItems >= D->maxItems) {
    D->maxItems *= 2;
    D->item = (char *) smartRealloc(D->item, D->itemSize * D->maxItems, 
                                    D->memType);
  }
  memcpy(D->item + (D->numItems++ * D->itemSize), p, D->itemSize);
}

void darraySet(Darray D, int idx, char *p) {
  if (D->maxItems <= idx) {
    D->maxItems = ((D->maxItems * 2) > idx) ? D->maxItems * 2 : idx + 1;
    D->item = (char *) smartRealloc(D->item, D->itemSize * D->maxItems,
                                    D->memType);
  }
  memcpy(D->item + (idx * D->itemSize), p, D->itemSize);
  if (D->numItems <= idx) D->numItems = idx + 1;
}

void shrinkDarray(Darray D) {
  if (D->maxItems > D->numItems) {
    D->maxItems = D->numItems;
    if (D->maxItems < 1) D->maxItems = 1;
    D->item = (char *) smartRealloc(D->item, D->itemSize * D->maxItems,
                                    D->memType);
  }
}

void clearDarray(Darray D) {
  D->numItems = 0;
}

void freeDarray(Darray D) {
  if (D) {
    smartFree(D->item, D->memType);
    smartFree(D, D->memType);
  }
}


/********************************** Randomness *******************************/

int lastSeed = 0;

#ifdef NO_DRAND48
double drand48(void) {
  return ((double) random()) / LONG_MAX;
}
#endif

void seedRand(unsigned int seed) {
#ifndef NO_DRAND48
  srand48((long) seed);
#endif /* MACHINE_WINNT */
  srandom(seed);
  lastSeed = seed;
}

void timeSeedRand(void) {
  unsigned int seed = getpid() * getUTime() * 31;
  seedRand(seed);
}

unsigned int getSeed(void) {
  return lastSeed;
}

int randInt(int max) {
  if (max < (1 << 16))
    return (int) ((random() >> 16) % max);
  else return (int) ((random()) % max);
}

real randReal(real mean, real range) {
  return (real) (drand48() * 2 - 1.L) * range + mean;
}

real randProb(void) {
  return (real) drand48();
}

/* Adapted from Numerical Methods in C, pg. 289 */
real randGaussian(real mean, real range) {
  static int iset = 0;
  static real gset, lmean, lrange;
  real fac, rsq, v1, v2;

  if (iset == 0 || lmean != mean || lrange != range) {
    do {
      v1 = randReal(0.0, 1.0);
      v2 = randReal(0.0, 1.0);
      rsq = v1 * v1 + v2 * v2;
    } while (rsq >= 1.0 || rsq == 0.0);
    fac = (real) sqrt((double) -2.0 * log((double) rsq) / rsq);
    gset = v1 * fac * range + mean;
    iset = 1;
    lmean = mean;
    lrange = range;
    return v2 * fac * range + mean;
  } else {
    iset = 0;
    return gset;
  }
}

void randSort(int *array, int n) {
  int i, j, temp;
  for (i = 0; i < (n - 1); i++) {
    j = randInt(n - i);
    temp = array[i];
    array[i] = array[i + j];
    array[i + j] = temp;
  }
}


/************************************* Time **********************************/

/* This handles the problem of accounting for wrap around */
unsigned long timeElapsed(unsigned long a, unsigned long b) {
  if (a <= b) return b - a;
  if (b > a - 1000) {
    printf("TimeElapsed problem %ld %ld\n", a, b);
    return 0;
  }
  return 4000000000ul - a + b;
}

/* Real time in milliseconds (mod 4B) */
unsigned long getTime(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((unsigned long) tv.tv_sec % 4000000) * 1000 + 
    (unsigned long) tv.tv_usec * 1e-3;
}

/* Real time in microseconds (mod 4B) */
unsigned long getUTime(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((unsigned long) tv.tv_sec % 4000) * 1000000 + 
    (unsigned long) tv.tv_usec;
}

/* Time is in seconds.  This prints it in hr/min or min/sec or sec format  */
void formatTime(unsigned long time, char *dest) {
  if (time > 3600)
    sprintf(dest, "%2luh %2lum", (time / 3600), 
	    ((time % 3600) / 60));
  else {
    if (time > 60) {
      sprintf(dest, "%2lum ", time / 60);
      time %= 60; 
   } else sprintf(dest, "    ");
    sprintf(dest + 4, "%2lus", time);
  }
}

flag printProgress(unsigned int i, unsigned int max) {
  static unsigned long last = 0;
  unsigned int now;
  int ProgressDelay = 30;
  if (last == 0 || i <= 1) last = getTime();
  else {
    now = getTime();
    if (timeElapsed(last, now) > ProgressDelay * 1000) {
      if (max) debug("    %4.1f%%\n", ((real) i * 100) / max);
      else debug("    %d\n", i);
      last = now;
      return TRUE;
    }
  }
  return FALSE;
}

LoopTimer newLoopTimer(int memType) {
  LoopTimer T = (LoopTimer) smartCalloc(1, sizeof(struct loopTimer), memType);
  T->start = T->last = getTime();
  return T;
}

void updateLoopTimer(LoopTimer T, int i, int total) {
  unsigned long now = getTime();
  real delta;
  if (i <= T->lasti) return;
  delta = (real) (now - T->last) / (i - T->lasti);
  if (T->mean == 0) T->mean = delta;
  else T->mean = (delta + 2 * T->mean) / 3;
  T->elapsed = (now - T->start) * 1e-3;
  T->left = T->mean * (total - i) * 1e-3;
  T->total = T->elapsed + T->left;
  T->last = now;
  T->lasti = i;
}

/************************************* Math **********************************/

int  imin(int a, int b) {return (a < b) ? a : b;}
int  imax(int a, int b) {return (a < b) ? b : a;}
real rmin(real a, real b) {return (a < b) ? a : b;}
real rmax(real a, real b) {return (a < b) ? b : a;}


/************************************* Binary IO *****************************/

flag readBinChar(FILE *file, char *c) {
  if (fread(c, sizeof(char), 1, file) == 1)
    return FALSE;
  return TRUE;
}

flag readBinShort(FILE *file, short *val) {
  short x;
  if (fread(&x, sizeof(short), 1, file) == 1) {
    *val = NTOHS(x);
    return FALSE;
  }
  return TRUE;
}

flag readBinInt(FILE *file, int *val) {
  int x;
  if (fread(&x, sizeof(int), 1, file) == 1) {
    *val = NTOHL(x);
    return FALSE;
  }
  return TRUE;
}

/* This reads a float in network order and converts to a real in host order. */
flag readBinReal(FILE *file, real *val) {
  int x;
  float y;
  if (fread(&x, sizeof(int), 1, file) == 1) {
    x = NTOHL(x);
    y = *((float *) &x);
    *val = (isNaNf(y)) ? NaN : (real) y;
    return FALSE;
  }
  return TRUE;
}

flag readBinDouble(FILE *file, double *val) {
  double v;
#ifdef LITTLE_END
  char b[8], *a;
#endif
  if (fread(&v, sizeof(double), 1, file) != 1) return TRUE;
#ifdef LITTLE_END
  a = (char *) (&v);
  b[0] = a[7]; b[1] = a[6]; b[2] = a[5]; b[3] = a[4];
  b[4] = a[3]; b[5] = a[2]; b[6] = a[1]; b[7] = a[0];
  *val = *((double *) b);
#else
  *val = v;
#endif
  return TRUE;
}

flag readBinFlag(FILE *file, flag *val) {
  char c;
  if (fread(&c, sizeof(char), 1, file) == 1) {
    *val = (flag) c;
    return FALSE;
  }
  return TRUE;
}

flag readBinString(FILE *file, String S) {
  int i = 0;
  do {
    if (i >= S->maxChars) 
      stringSize(S, 2 * S->maxChars);
    if (fread(S->s + i, sizeof(char), 1, file) != 1) return TRUE;
  } while (S->s[i++] != '\0');
  S->numChars = i - 1;
  return FALSE;
}

flag writeBinChar(FILE *file, char c) {
  if (fwrite(&c, sizeof(char), 1, file)) return TRUE;
  return FALSE;
}

flag writeBinShort(FILE *file, short x) {
#ifdef LITTLE_END
  short y = HTONS(x);
  if (fwrite(&y, sizeof(short), 1, file) != 1) return TRUE;
#else
  if (fwrite(&x, sizeof(short), 1, file) != 1) return TRUE;
#endif
  return FALSE;
}

flag writeBinInt(FILE *file, int x) {
#ifdef LITTLE_END
  int y = HTONL(x);
  if (fwrite(&y, sizeof(int), 1, file) != 1) return TRUE;
#else
  if (fwrite(&x, sizeof(int), 1, file) != 1) return TRUE;
#endif
  return FALSE;
}

/* This takes a real in host order and writes a float in network order. */
flag writeBinReal(FILE *file, real r) {
#ifdef FLOAT_REAL
#  ifdef LITTLE_END
  int y = HTONL(*((int *) &r));
  if (fwrite(&y, sizeof(int), 1, file) != 1) return TRUE;
#  else
  if (fwrite(&r, sizeof(int), 1, file) != 1) return TRUE;
#  endif /* LITTLE_END */
#else /* FLOAT_REAL */
  float x = (isNaN(r)) ? NaNf : (float) r;
#  ifdef LITTLE_END
  int y = HTONL(*((int *) &x));
  if (fwrite(&y, sizeof(int), 1, file) != 1) return TRUE;
#  else
  if (fwrite(&x, sizeof(int), 1, file) != 1) return TRUE;
#  endif /* LITTLE_END */
#endif /* FLOAT_REAL */
  return FALSE;
}

flag writeBinDouble(FILE *file, double r) {
#ifdef LITTLE_END
  char b[8], *a;
#endif
  double x = (isNaN(r)) ? NaNd : r;
#ifdef LITTLE_END
  a = (char *) (&x);
  b[0] = a[7]; b[1] = a[6]; b[2] = a[5]; b[3] = a[4];
  b[4] = a[3]; b[5] = a[2]; b[6] = a[1]; b[7] = a[0];
  if (fwrite(b, 8, 1, file) != 1) return TRUE;
#else
  if (fwrite(&x, sizeof(double), 1, file) != 1) return TRUE;
#endif
  return FALSE;
}

flag writeBinFlag(FILE *file, flag f) {
  char c = (char) f;
  if (fwrite(&c, sizeof(char), 1, file)) return TRUE;
  return FALSE;
}

/* len == strlen(s) or it will be computed if it is negative. */
void writeBinString(FILE *file, char *s, int len) {
  char x = '\0';
  if (s) {
    if (len < 0) len = strlen(s);
    fwrite(s, sizeof(char), len + 1, file);
  } else fwrite(&x, sizeof(char), 1, file);
}

void writeReal(FILE *file, real r, char *pre, char *post) {
  if (isNaN(r)) 
    fprintf(file, "%s%s%s", pre, NO_VALUE, post);
  else
    fprintf(file, "%s%g%s", pre, r, post);
}

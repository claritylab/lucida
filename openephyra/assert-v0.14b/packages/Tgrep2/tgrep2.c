/*
   Tgrep2
   Copyright (C) 2001-2002, Douglas Rohde
   Questions or bug reports should be addressed to dr+tg@cs.cmu.edu

   Acknowledgements:
   Thanks to Eric Joanis for suggesting the ability to handle comments and
   more than 255 children per node and for writing the first implementation of
   those features.

   Also thanks to Eric for suggesting and implementing optional links.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <regex.h>
#include <netinet/in.h>
#include <drutils.h>
#include <hash.h>

/********************************** Constants ********************************/

/* Strings */
#define VERSION "1.14"
#define COOKIE1 "Tgrep2 Corpus File"
#define COOKIE2 "Tgrep2 Corpus File 2"
#define NO_TREE "<none>"

/* Initial Values */
#define INIT_HASH     100000
#define INIT_BUFFER   4096
#define INIT_TREES    2048
#define INIT_STREES   1200
#define PATTERN_BUF   256
#define INIT_KIDS     16
#define INIT_NAMES    8
#define INIT_PATTERNS 8

/* Macros */
#define SENTENCE(x) ((SentenceNum - 1 + TotalStored + x) % TotalStored)

/* Enumerations */
enum memTypes {MEM_TEMP, MEM_LINK, MEM_TREE, MEM_WORD, MEM_STRING, 
	       MEM_PATTERN, MEM_FORMAT, MEM_SENTENCE, MEM_MACRO, MEM_TYPES};

enum linkTypes {DOM_LEFT, IS_DOM_LEFT, DOM_RIGHT, IS_DOM_RIGHT, DOM_ONLY, 
		IS_DOM_ONLY, DOMINATES, IS_DOMINATED, HAS_CHILD_LEFT, 
		HAS_CHILD_RIGHT, HAS_CHILD_ONLY, HAS_CHILD, IS_CHILD_LEFT, 
		IS_CHILD_RIGHT, IS_CHILD_ONLY, IS_CHILD, IS_PRIOR, IS_AFTER, 
		IMM_PRIOR, IMM_AFTER, SIS_PRIOR, SIS_AFTER, SIS_IMM_PRIOR, 
		SIS_IMM_AFTER, HAS_SIS, HAS_CHILD_NUM, IS_CHILD_NUM,
		EQUAL, SAME_NAME, LINK_TYPES};

enum outputTypes {F_STRING, F_FILENAME, F_SNUM, F_PNUM, F_SMATCHNUM, 
		  F_PMATCHNUM, F_COMMENT, F_HEAD, F_MARKED, F_WHOLE, F_LABEL, 
		  F_CONTEXT};

enum outputFormat {F_SHORT, F_LONG, F_TERMINAL, F_NUM, F_CODE, F_TOPNODE, 
                   F_FIRST, F_LAST, F_LENGTH, F_DEPTH};

enum linkOperators {AND, OR};

enum matchModes {M_FIRST, M_ALL, M_FILTER};

enum wordPatTypes {W_ALL, W_NONE, W_CONST, W_REGEXP};

/********************************* Structures ********************************/

typedef struct ltype *Ltype;
typedef struct tree *Tree;
typedef struct sentence *Sentence;
typedef struct word *Word;
typedef struct node *Node;
typedef struct link *Link;
typedef struct wpat *Wpat;
typedef struct pattern *Pattern;
typedef struct format *Format;
typedef regex_t *RegExp;

/* This contains one of the registered link types. */
struct ltype {
  int   type;
  char *code;
  int   len;
  int   cost;
  char (*match)(Link, Tree); /* Searches for a matching tree node. */
  char (*check)(Link, Tree); /* Checks two tree nodes for a match. */
};

/* This holds a node in the sentence parse tree. */
struct tree {
  Word  word;
  Tree  parent;
  int   kidNum;      /* The child number of this tree for its parent. */
  int   preNum;      /* The pre-order numbering of the node (starts w/ 1). */
  int   postNum;     /* The post-order numbering of the node (starts w/ 1). */
  int   length;      /* Number of terminals in this tree. */
  unsigned short numKids;
  unsigned short maxKids;
  Tree *kid;
  char *match;       /* Holds a cache of nodes whose trees match. */
};

/* This holds all the nodes in a complete sentence. */
struct sentence {
  int   numTrees;    /* Number of trees in the array. */
  int   maxTrees;    /* Size of the tree array. */
  Tree *tree;        /* Array of tree pointers. */
  char *comment;
};

/* This stores a symbol name for the sentence parse trees. */
struct word {
  char *name;
  int   num;
  char *match;       /* Holds a cache of nodes that immediately match. */
};

/* This is a node in the pattern tree. */
struct node {
  int   num;         /* The node number in creation order. */
  int   preNum;      /* The pre-order numbering of the node (starts w/ 1). */
  int   postNum;     /* The post-order numbering of the node (starts w/ 1). */

  int   numNames;
  Wpat  names;       /* The tree node names that this matches. */
  char *label;       /* The label of the node, if specified. */

  Tree  tree;        /* What the node is matched to. */

  char  not;         /* If true, node name matching is inverted. */
  char  cacheable;   /* Not cacheable if its subtree has any back-links. */
  char  print;       /* Whether the node is marked for printing. */

  int   inLinks;
  Link  link;
  Link  parentLink;
  Node  copy;        /* Points from an original to a copy. */
  Node  nextMarked;  /* The next node marked for printing if this is. */
};

/* This is either a pointer to a node or is a conjunction or disjunction of 
   other links. */
struct link {
  Node  node;        /* NULL if this is a container. */
  Ltype ltype;
  char  equal;       /* If true, the link can be matched by the node itself. */
  char  backLink;    /* True if this is a back edge, forming a cycle. */
  char  not;         /* If true, this link must not match. */
  char  optional;    /* True if ? preceedes this link--optional matching. */
  char  operator;    /* AND or OR */

  short childNum;    /* Used for IS_CHILD_NUM and HAS_CHILD_NUM */
  int   numLinks;
  Link *link;
  Link  parentLink;  /* Only one of these is set. */
  Node  parentNode;
};

/* This is a pattern that matches a symbol, either a constant or regexp. */
struct wpat {
  char  type;
  char *string;
  Word  word;
  RegExp regExp;     /* NULL if a constant. */
};

/* This contains information about a complete pattern. */
struct pattern {
  int   num;         /* Starting from 0. */
  int   matchNum;    /* Subtree match number on current sentence. */
  Node  head;
  int   numNodes;
  Node *node;
  int   numPrints;
  Node *print;
  HashTable labels;
};

/* This defines a field in a formatted output expression. */
struct format {
  char  type;
  char  format;
  char *string;  /* Used for constants and has the format string for ints. */
  char *label;   /* Used for labels. */
  int   num;     /* Used for before and after. */
  Format next;
};

/******************************* Global Variables ****************************/

/* Definitions */
char SpecialSymbols[] = " \t;:.,&|<>()[]$!?@%`^=";

/* Main Structures */
Ltype Ltypes;              /* A linked list of link types. */

int NumPatterns = 0;       /* The number of patterns. */
int TotalNodes = 0;        /* The total nodes across all patterns. */
Pattern *Patterns = NULL;  /* An array of or'd patterns. */
Pattern CurrentPattern;    /* A pointer to the current pattern. */

int NumSentences = 0;      /* The number of sentences in the corpus. */
int NumWords = 0;          /* The number of different words in the corpus. */
Word Words;                /* An array of all different words in the corpus. */
HashTable WordHash;        /* Holds words while preparing a corpus. */
HashTable Macros = NULL;   /* Stores the pattern-building macros. */
Word *WordList;            /* Array of pointers to words used in preparing. */

Tree *Trees;               /* Array of unused tree nodes. */
int MaxTrees = 0;          /* The size of the trees array. */
int NumTrees = 0;          /* The number of unused trees in the Trees array. */
Tree Top = NULL;           /* The top node in the current parse tree. */

String Buffer = NULL;      /* Space usable during parsing. */

Format SentBeginFormat = NULL, 
  SentenceFormat = NULL, 
  MatchFormat = NULL, 
  DefaultFormat = NULL;

/* User Variables */
int  TMatchMode     = M_FIRST;
int  PMatchMode     = M_ALL;
char DefFormat      = F_SHORT;
char WholeSentence  = FALSE;
char ReorderLinks   = TRUE;
int  ReportInterval = 0;
int  NumBefore = 0, NumAfter = 0, TotalStored = 1;
char *CorpusFile;
char PrintComments  = FALSE; /* Should sent. comments be stored or printed */
char HasComments    = FALSE; /* True if the corpus contains comments. */
char ManyKids       = FALSE; /* If true, numKids stored using a short int. */
char IgnoreCase     = FALSE; /* If true, case doesn't matter in matches. */
char ComputeLengths = FALSE;

/* Working Values */
char Done;
int  PreNum, PostNum;
int  SentenceNum = 0, SMatchNum = 0, TMatchNum = 0;
Sentence CurrentSentence;
Sentence *Sent;


/****************************** Helper Functions *****************************/

RegExp regExpComp(char *pattern) {
  RegExp E;
  int flags = REG_EXTENDED | REG_NOSUB;
  if (IgnoreCase) flags |= REG_ICASE;
  E = (RegExp) smartMalloc(sizeof *E, 0);
  if (regcomp(E, pattern, flags) == 0) return E;
  else fatalError("Bad regular expression: %s", pattern);
  return NULL;
}

int regExpMatch(RegExp E, char *string) {
  if (regexec(E, string, 0, NULL, 0) == 0) return 1;
  return 0;
}

void regExpFree(RegExp E) {
  if (!E) return;
  regfree(E);
  smartFree(E, 0);
}

void reportProgress(int i, int max) {
  static unsigned long last = 0;
  unsigned int now;
  if (ReportInterval <= 0) return;
  if (last == 0 || i <= 1) last = getTime();
  else {
    now = getTime();
    if (timeElapsed(last, now) > ReportInterval * 1000) {
      if (max) debug("%4.1f%%\n", ((real) i * 100) / max);
      else debug("%d\n", i);
      last = now;
    }
  }
}


/******************************* Corpus Reading ******************************/

FILE *openCorpusFile(char *filename) {
  int i, k;
  unsigned char j, version = 0;
  FILE *file = fatalReadFile(filename);
  String S = newString(32, MEM_STRING), N = newString(32, MEM_STRING);
  Word W, P = NULL;

  readBinString(file, S);
  if (!strcmp(S->s, COOKIE1)) version = 1;
  else if (!strcmp(S->s, COOKIE2)) version = 2;
  else fatalError("\"%s\" is not a TGrep2 corpus file.\n"
		  "Use the -p option to create one.", filename);
  HasComments = FALSE;
  if (version == 2) {
    readBinString(file, S);
    for (i = 0; S->s[i]; i++) {
      if (S->s[i] == 'C') HasComments = TRUE;
      else if (S->s[i] == 'K') ManyKids = TRUE;
      else fatalError("\"%s\" has an unrecognized type flag: %c\n",
		      filename, S->s[i]);
    }
  }

  readBinInt(file, &NumWords);
  Words = smartMalloc(NumWords * sizeof(struct word), MEM_WORD);
  /*  WordHash = hashTableCreate(NumWords, 2, hashString, compareStrings, 
      MEM_WORD); */
  for (i = 0; i < NumWords; i++) {
    W = Words + i;
    readBinChar(file, (char *) &j);
    clearString(N);
    for (k = 0; k < j; k++)
      stringCat(N, P->name[k]);
    readBinString(file, S);
    stringAppend(N, S->s);
    W->name = copyString(N->s, MEM_WORD);
    W->num = i;
    W->match = smartMalloc(TotalNodes * sizeof(char), MEM_WORD);
    memset(W->match, -1, TotalNodes);
    /* hashTableInsert(WordHash, W->name, W); */
    P = W;
  }
  readBinInt(file, &NumSentences);
  freeString(S);
  freeString(N);
  return file;
}

Sentence newSentence(void) {
  Sentence S = smartCalloc(1, sizeof(struct sentence), MEM_SENTENCE);
  S->maxTrees = INIT_STREES;
  S->tree = smartMalloc(INIT_STREES * sizeof(Tree), MEM_SENTENCE);
  S->comment = NULL;
  return S;
}

void freeSentence(Sentence S) {
  smartFree(S->tree, MEM_SENTENCE);
  smartFree(S->comment, MEM_SENTENCE);
  smartFree(S, MEM_SENTENCE);
}

Tree getTree(void) {
  Tree T;
  if (NumTrees == 0) {
    T = smartCalloc(1, sizeof(struct tree), MEM_TREE);
    if (TotalNodes)
      T->match = smartMalloc(TotalNodes * sizeof(char), MEM_TREE);
    T->kid = smartMalloc(INIT_KIDS * sizeof(Tree), MEM_TREE);
    T->maxKids = INIT_KIDS;
  } else {
    T = Trees[--NumTrees];
    if (TotalNodes && !T->match)
      T->match = smartMalloc(TotalNodes * sizeof(char), MEM_TREE);
  }
  return T;
}

void returnTree(Tree T) {
  if (!T) return;
  if (MaxTrees == 0) {
    MaxTrees = INIT_TREES;
    Trees = smartMalloc(MaxTrees * sizeof(Tree), MEM_TREE);
  } else if (NumTrees >= MaxTrees) {
    MaxTrees *= 2;
    Trees = smartRealloc(Trees, MaxTrees * sizeof(Tree), MEM_TREE);
  }
  Trees[NumTrees++] = T;
}

void returnSentence(Sentence S) {
  int i;
  for (i = 0; i < S->numTrees; i++) {
    returnTree(S->tree[i]);
    S->tree[i] = NULL;
  }
  S->numTrees = 0;
}

void buildSentence(Sentence S, FILE *file) {
  int i, v, w, postNum = 1, bytes;
  unsigned short kids, k;
  unsigned short trees;
  Tree T, P = NULL;
  char *p;
  
  if (HasComments) {
    String comment = newString(64, MEM_TEMP);
    readBinString(file, comment);
    S->comment = (comment->numChars) ? copyString(comment->s, MEM_SENTENCE) 
      : NULL;
    freeString(comment);
  }
  if (S->numTrees) returnSentence(S);

  fread(&trees, 1, sizeof(unsigned short), file);
  S->numTrees = ntohs(trees);
  if (S->maxTrees < S->numTrees) {
    S->maxTrees = S->numTrees;
    S->tree = smartRealloc(S->tree, S->maxTrees * sizeof(Tree), MEM_SENTENCE);
  }

  if (ManyKids)
    bytes = (sizeof(int) + sizeof(short)) * S->numTrees;
  else bytes = (sizeof(int) + sizeof(char)) * S->numTrees;
  stringSize(Buffer, bytes);
  if (fread(Buffer->s, 1, bytes, file) != bytes)
    fatalError("The corpus file was truncated.");
  p = Buffer->s;

  for (i = 0; i < S->numTrees; i++) {
    T = S->tree[i] = getTree();
    T->preNum = i + 1;
    T->postNum = 0;
    T->length = 0;
    if (P) {
      for (; P->postNum == P->numKids; P = P->parent) P->postNum = postNum++;
      P->kid[P->postNum] = T;
      T->kidNum = P->postNum++;
    }
    T->parent = P;
    memset(T->match, -1, TotalNodes);
    
    memcpy((char *) (&v), p, sizeof(int));
    p += sizeof(int);
    /* w = ntohl(*(((int *) p)++)); */
    w = ntohl(v);
    if (w < 0 || w >= NumWords) 
      fatalError("Word %d out of range in sentence %d", w, SentenceNum);

    T->word = Words + w;
    if (ManyKids) {
      memcpy((char *) (&k), p, sizeof(short));
      p += sizeof(short);
      kids = ntohs(k);
    } else kids = *(p++);

    T->numKids = kids;
    if (kids > T->maxKids) {
      smartFree(T->kid, MEM_TREE);
      T->kid = smartMalloc(kids * sizeof(Tree), MEM_TREE);
      T->maxKids = kids;
    }
    P = T;
  }
  for (; P; P = P->parent) P->postNum = postNum++;
  if (ComputeLengths) {
    for (i = S->numTrees - 1; i >= 0; i--) {
      T = S->tree[i];
      if (T->numKids == 0) T->length = 1;
      if (T->parent) T->parent->length += T->length;
    }
  }
}


/******************************** Tree Printing ******************************/

int treeFirstTerminal(Tree T) {
  int i, start;
  Tree P = T->parent;
  if (!P) return 1;
  start = treeFirstTerminal(P);
  for (i = T->kidNum - 1; i >= 0; i--)
    start += P->kid[i]->length;
  return start;
}

int treeDepth(Tree T) {
  int i, depth = 1;
  if (!T->numKids) return 1;
  for (i = 0; i < T->numKids; i++) {
    int d = treeDepth(T->kid[i]);
    if (d + 1 > depth) depth = d + 1;
  }
  return depth;
}

void printShortTree(Tree T) {
  int i;
  if (T->numKids) {
    putchar('(');
    fputs(T->word->name, stdout);
    for (i = 0; i < T->numKids; i++) {
      putchar(' ');
      printShortTree(T->kid[i]);
    }
    putchar(')');
  } else fputs(T->word->name, stdout);
}

void printLongTree(Tree T, int depth) {
  int i;
  if (depth && T->kidNum > 0) {
    putchar('\n');
    for (i = 0; i < depth; i++)
      putchar(' ');
  }
  if (T->numKids) {
    putchar('(');
    depth++;
  }
  fputs(T->word->name, stdout);
  depth += strlen(T->word->name);
  if (T->numKids) {
    putchar(' ');
    depth++;
    for (i = 0; i < T->numKids; i++)
      printLongTree(T->kid[i], depth);
    putchar(')');
  }
}

char printTerminals(Tree T, char first) {
  int i;
  if (T->numKids) {
    for (i = 0; i < T->numKids; i++)
      first = printTerminals(T->kid[i], first);
  } else {
    if (!first) putchar(' '); else first = FALSE;
    fputs(T->word->name, stdout);
  }
  return first;
}

void printTree(Tree T, Format F) {
  if (!T) {
    if (F->format == F_NUM) putchar('0');
    else if (F->format == F_CODE) printf("%d:0", SentenceNum);
    else fputs(NO_TREE, stdout);
    return;
  }
  switch (F->format) {
  case F_SHORT: 
    printShortTree(T); break;
  case F_LONG:
    printLongTree(T, 0); break;
  case F_TERMINAL:
    printTerminals(T, TRUE); break;
  case F_NUM:
    printf(F->string, T->preNum); break;
  case F_CODE:
    printf("%d:%d", SentenceNum, T->preNum); break;
  case F_TOPNODE:
    printf("%s", T->word->name); break;
  case F_FIRST:
    printf("%d", treeFirstTerminal(T)); break;
  case F_LAST:
    printf("%d", treeFirstTerminal(T) + T->length - 1); break;
  case F_LENGTH:
    printf("%d", T->length); break;
  case F_DEPTH:
    printf("%d", treeDepth(T)); break;
  }
}

/* This checks to see if a node is actually matched to a tree in the current
   matching.  Hopefully faster because it's non-recursive. */
char isPrintable(Node P) {
  Link L = NULL;
  while (1) {
    if (P) {
      if (!P->tree) return FALSE;
      if (!P->parentLink) return TRUE;
      L = P->parentLink;
      P = NULL;
    } else {
      if (L->not) return FALSE;
      if (L->parentLink) L = L->parentLink;
      else {
	P = L->parentNode;
	L = NULL;
      }
    }
  }
}

void printNodeTree(Node P, Format F) {
  if (!P || !isPrintable(P)) printTree(NULL, F);
  else printTree(P->tree, F);
}

/* Prints trees matched to nodes marked for printing or the head node. */
void printMarkedTrees(Format F) {
  Pattern R = CurrentPattern;
  int i;
  if (!R) {printTree(NULL, F); return;}
  if (!R->numPrints) printNodeTree(R->head, F);
  else for (i = 0; i < R->numPrints; i++) {
    if (i > 0) putchar('\n');
    printNodeTree(R->print[i], F);
  }
}

void printComment(Sentence S) {
  if (S->comment) puts(S->comment);
}


/**************************** Macro Substitution *****************************/

void addMacro(char *macro, char *value) {
  if (!Macros) Macros = hashTableCreate(64, 2, hashString, compareStrings, 
					MEM_MACRO);
  hashTableInsert(Macros, copyString(macro, MEM_MACRO), 
		  copyString(value, MEM_MACRO), FALSE);
}

char *getMacro(char *macro) {
  return (char *) hashTableLookup(Macros, macro);
}

void macroSubstitute(String A, String B) {
  char *s, p = '\0';
  clearString(B);
  for (s = A->s; *s; p = *s, s++) {
    if (*s == '@' && p == '\\') {
      B->numChars--;
      stringCat(B, '@');
    } else if (*s == '@' && !strchr(SpecialSymbols, s[1])) {
      int i;
      char *value;
      clearString(Buffer);
      for (i = 1; !strchr(SpecialSymbols, s[i]); i++)
	stringCat(Buffer, s[i]);
      s += i - 1;
      if (!(value = getMacro(Buffer->s)))
	fatalError("Undefined macro: \"%s\"", Buffer->s);
      stringAppend(B, value);
    } else stringCat(B, *s);
  }
}

/****************************** Parsing Patterns *****************************/

/* Assumes P is cleared. */
char loadPatternFile(char *filename, String P) {
  FILE *f;
  char comment = FALSE, *p = Buffer->s;
  
  if (!(f = readFile(filename))) return FALSE;
  while (fgets(p, INIT_BUFFER, f)) {
    if (p[0] == '#') comment = TRUE;
    if (comment) {
      int len = strlen(p);
      char last = (len) ? p[len - 1] : '\0';
      if (last == '\n') comment = FALSE;
      continue;
    }
    stringAppend(P, p);
    if (P->numChars && P->s[P->numChars - 1] == '\n') {
      P->s[P->numChars - 1] = ' ';
    }
  }
  closeFile(f);
  return TRUE;
}

void skipBlank(char **sp) {
  char *s = *sp;
  while (*s && isspace(*s)) s++;
  *sp = s;
}

Link newLink(void) {
  return smartCalloc(1, sizeof(struct link), MEM_LINK);
}

void freeLink(Link L) {
  smartFree(L->link, MEM_LINK);
  smartFree(L, MEM_LINK);
}

void addLink(Link P, Link L) {
  P->numLinks++;
  P->link = smartRealloc(P->link, P->numLinks * sizeof(Link), MEM_LINK);
  P->link[P->numLinks - 1] = L;
}

char matchesCode(char *s, Ltype T) {
  char code[8];
  int i;
  for (i = 0; i < T->len; i++) {
    if (s[i] == '{') code[i] = '<';
    else if (s[i] == '^') code[i] = '<';
    else if (s[i] == '}') code[i] = '>';
    else if (s[i] == '%') code[i] = '$';
    else code[i] = s[i];
  }
  code[i] = '\0';
  if (!strcmp(T->code, code)) return TRUE;
  return FALSE;
}

Link parseLinkType(char **sp) {
  Link L = newLink();
  char *s;
  int i, type;
  skipBlank(sp);

  s = *sp;

  for (i = 0; i < LINK_TYPES; i++) {
    Ltype T = Ltypes + i;
    if (matchesCode(s, T)) {
      s += T->len;
      L->ltype = T;
      break;
    }
  }
  if (!L->ltype) fatalError("Bad relationship in pattern %d here: \"%s\"",
                            CurrentPattern->num, *sp);

  /* Handle Nth child stuff. */
  type = L->ltype->type;
  if ((type == HAS_CHILD || type == IS_CHILD) && (*s == '-' || isdigit(*s))) {
    if (*s == '-' && !isdigit(s[1])) {
      L->childNum = -1;
      s++;
    } else {
      int shift, num;
      if (sscanf(s, "%d%n", &num, &shift) != 1)
	fatalError("Error parsing link child num here: \"%s\"", s);
      L->childNum = num;
      s += shift;
    }
    if (type == HAS_CHILD) L->ltype = Ltypes + HAS_CHILD_NUM;
    else L->ltype = Ltypes + IS_CHILD_NUM;
  } else if (type == HAS_CHILD_LEFT) {
    L->ltype = Ltypes + HAS_CHILD_NUM;
    L->childNum = 1;
  } else if (type == HAS_CHILD_RIGHT) {
    L->ltype = Ltypes + HAS_CHILD_NUM;
    L->childNum = -1;
  } else if (type == IS_CHILD_LEFT) {
    L->ltype = Ltypes + IS_CHILD_NUM;
    L->childNum = 1;
  } else if (type == IS_CHILD_RIGHT) {
    L->ltype = Ltypes + IS_CHILD_NUM;
    L->childNum = -1;
  }
  if (*s == '=') {
    if (L->ltype->type != EQUAL) L->equal = TRUE;
    s++;
  }
  
  *sp = s;
  return L;
}

Node newNode(void) {
  Node P = smartCalloc(1, sizeof(struct node), MEM_PATTERN);
  P->num = TotalNodes++;
  return P;
}

Node lookupLabel(char *label) {
  return hashTableLookup(CurrentPattern->labels, label);
}

void registerLabel(Node P) {
  hashTableInsert(CurrentPattern->labels, P->label, P, FALSE);
}

Node parsePatternName(char **sp) {
  Node P = NULL;
  Wpat names = NULL, V;
  int numNames = 0, maxNames = INIT_NAMES;
  char *s = *sp, *label = NULL, not = FALSE;
  String S = newString(32, MEM_TEMP);

  skipBlank(&s);

  if (*s == '=') { /* This is just a labeled node. */
    clearString(S);
    for (s++; !strchr(SpecialSymbols, *s); s++)
      stringCat(S, *s);
    if (!(P = lookupLabel(S->s)))
      label = copyString(S->s, MEM_PATTERN);
  } else {
    if (*s == '!') {
      not = TRUE;
      s++;
    }
    names = smartMalloc(maxNames * sizeof(struct wpat), MEM_PATTERN);
    while (*s && !strchr(SpecialSymbols, *s)) {
      if (numNames == maxNames) {
	maxNames *= 2;
	names = smartRealloc(names, maxNames*sizeof(struct wpat), MEM_PATTERN);
      }
      V = names + numNames++;
      memset(V, 0, sizeof(struct wpat));
      V->type = W_CONST;

      clearString(S);
      if (*s == '/') {
	/* This is a regexp. */
	for (s++; *s && (*s != '/' || s[-1] == '\\'); s++)
	  stringCat(S, *s);
	if (*s != '/') 
	  fatalError("Unterminated regular expression in pattern %d", 
		     CurrentPattern->num);
	s++;
	V->regExp = regExpComp(S->s);
	V->type = W_REGEXP;
      } else if (*s == '"') {
	/* This is a quoted string. */
	for (s++; *s != '"' || s[-1] == '\\'; s++) {
	  if (*s == '"') S->s[S->numChars - 1] = '"';
	  else stringCat(S, *s);
	}
	s++;
      } else {
	/* This is a normal symbol. */
	for (; !strchr(SpecialSymbols, *s); s++)
	  stringCat(S, *s);
      }
      if (*s == '|' && s[1] && !strchr(SpecialSymbols, s[1])) s++;
      
      V->string = copyString(S->s, MEM_PATTERN);
      if (V->type != W_REGEXP && (!strcmp(S->s, "__") || !strcmp(S->s, "*")))
	V->type = W_ALL;
    }
    if (*s == '=') {
      clearString(S);
      for (s++; !strchr(SpecialSymbols, *s); s++)
	stringCat(S, *s);
      if (!(P = lookupLabel(S->s)))
	label = copyString(S->s, MEM_PATTERN);
    }
  }
  
  if (!P) {
    P = newNode();
    if (label) {
      P->label = label;
      registerLabel(P);
    } else if (!names)
      fatalError("Pattern %d is truncated or has a node with no name.", 
		 CurrentPattern->num);
  } else {
    if (names && P->names) 
      fatalError("Label \"%s\" is used to refer to two different nodes", 
		 P->label);
  }
  if (numNames) {
    P->numNames = numNames;
    P->names = names;
    P->not = not;
  }

  freeString(S);
  *sp = s;
  return P;
}

Node parsePatternNode(char **sp, char top);

/* This removes useless links in the boolean formula. */
Link cleanLink(Link L) {
  Link C;
  if (L->node) return L;
  if (L->numLinks == 0) return NULL;
  if (L->numLinks == 1) {
    C = L->link[0];
    freeLink(L);
    return cleanLink(C);
  } else {
    int i;
    for (i = 0; i < L->numLinks; i++)
      L->link[i] = cleanLink(L->link[i]);
  }
  return L;
}

Link parsePatternLinks(char **sp) {
  Link O = newLink(), L, N;
  char *s, not, optional;

  L = O;
  O->operator = OR;
  skipBlank(sp);
  s = *sp;
  while (*s && *s != ')' && *s != ']' && *s != ';' && *s != ':') {
    if (L->numLinks) {
      if (*s == '|') {
	L = O;
	s++;
      } else {
	if (L == O) {
	  L = newLink();
	  L->operator = AND;
	  addLink(L, O->link[O->numLinks - 1]);
	  O->link[O->numLinks - 1] = L;
	}
	if (*s == '&') s++;
      }
    }
    skipBlank(&s);

    not = optional = FALSE;
    while (*s == '?' || *s == '!' || *s == '@') {
      if (*s == '?') optional = TRUE;
      else not = TRUE;
      s++;
    }
    if (not && optional)
      fatalError("A link is marked both \"optional\" and \"not\", which doesn't make sense.");
    if (*s == '[') {
      s++;
      N = parsePatternLinks(&s);
      if (*s == ']') s++;
      else fatalError("Missing close-bracket in pattern here: \"%s\"", s);
    } else {
      N = parseLinkType(&s);
      N->node = parsePatternNode(&s, FALSE);
      N->node->inLinks++;
    }
    N->not = not;
    N->optional = optional;
    addLink(L, N);
    
    skipBlank(&s);
  }

  *sp = s;
  return cleanLink(O);
}

Node parsePatternNode(char **sp, char top) {
  char print = FALSE, *last;
  Node P;
  int depth = 0;
  skipBlank(sp);
  if (!**sp || **sp == ':' || **sp == ';') return NULL;

  while (**sp == '(' || **sp == '`') {
    if (**sp == '`') print = TRUE;
    else if (**sp == '(') depth++;
    (*sp)++;
    skipBlank(sp);
  }
  P = parsePatternName(sp);
  skipBlank(sp);
  while (**sp && (depth > 0 || top)) {
    last = *sp;
  
    if (**sp == ';' || **sp == ':') {
      if (depth > 0) 
        fatalError("Syntax error in pattern %d here: \"%s\"", 
                   CurrentPattern->num, *sp);
      else break;
    }

    if (**sp == ')') {
      if (depth <= 0) 
        fatalError("Unbalanced parentheses in pattern %d here: \"%s\"", 
                   CurrentPattern->num, *sp);
      depth--;
      (*sp)++;
    } else {
      Link L = parsePatternLinks(sp);
      if (P->link) {
	if (!P->link->node && P->link->operator == AND) {
          addLink(P->link, L);
        } else {
	  Link N = newLink();
	  N->operator = AND;
	  addLink(N, P->link);
	  addLink(N, L);
	  P->link = N;
        }
      } else P->link = L;
    }
    skipBlank(sp);
    if (last == *sp) 
      fatalError("Syntax error in pattern %d here: \"%s\"", 
                 CurrentPattern->num, *sp);
  }
  if (depth > 0) 
    fatalError("Unbalanced parentheses in pattern %d.", 
               CurrentPattern->num);

  if (print && !P->print) P->print = TRUE;
  skipBlank(sp);
  return P;
}


int linkCost(Link L) {
  int cost;
  if (L->node) cost = 2 * L->ltype->cost;
  else if (L->operator == AND) cost = 10;
  else cost = 20;
  if (L->not) cost++;
  if (L->optional) cost += 100;
  return cost;
}

int compareLinks(const void *a, const void *b) {
  Link A = *(Link *) a, B = *(Link *) b;
  return linkCost(A) - linkCost(B);
}

String printNodeToString(Node P);
int preparePattern(Node P, char pos);

char willPrint(Format F, char *label) {
  if (!F) return FALSE;
  if (F->type == F_LABEL && !strcmp(label, F->label))
    return TRUE;
  return willPrint(F->next, label);
}
char willBePrinted(char *label) {
  if (willPrint(MatchFormat, label)) return TRUE;
  return FALSE;
}

Node copyPattern(Node P);
Link copyLinks(Link L) {
  int i;
  Link C;
  if (!L) return NULL;
  if (L->backLink) fatalError("In pattern %d, a crossing edge points to a labeled subtree that contains a back-edge.  This is not allowed.", CurrentPattern->num);
  C = newLink();
  memcpy(C, L, sizeof(struct link));
  C->backLink = FALSE;
  if (L->numLinks) {
    C->link = smartMalloc(L->numLinks * sizeof(Link), MEM_LINK);
    for (i = 0; i < L->numLinks; i++) 
      C->link[i] = copyLinks(L->link[i]);
  }
  if (L->node) C->node = copyPattern(L->node);
  return C;
}

Node copyPattern(Node P) {
  int num;
  Node C = newNode();
  num = C->num;
  /* CurrentPattern->numNodes--; */
  memcpy(C, P, sizeof(struct node));
  C->num = num;
  C->preNum = C->postNum = 0;
  if (P->label) {
    String S = newString(32, MEM_TEMP);
    stringAppend(S, P->label);
    do {
      stringCat(S, '+');
    } while (lookupLabel(S->s));
    C->label = copyString(S->s, MEM_PATTERN);
    registerLabel(C);
    freeString(S);
  }
  C->link = copyLinks(P->link);
  P->copy = C;
  return C;
}

int prepareLinks(Link L, char pos) {
  int bh = 1000000;
  if (L->not) pos = FALSE;
  if (L->node) {
    if (L->node->postNum) {
      while (L->node->copy) L->node = L->node->copy;
      if (L->node->postNum) L->node = copyPattern(L->node);
    }
    if (L->node->preNum) {
      L->backLink = TRUE;
      if (L->node->preNum < bh) bh = L->node->preNum;
    } else {
      bh = preparePattern(L->node, pos);
      L->node->parentLink = L;
    }
  } else {
    int i;
    if (ReorderLinks)
      qsort(L->link, L->numLinks, sizeof(Link), compareLinks);
    for (i = 0; i < L->numLinks; i++) {
      int h = prepareLinks(L->link[i], pos);
      L->link[i]->parentLink = L;
      if (h < bh) bh = h;
    }
  }
  return bh;
}

/* This sorts the links for efficiency, computes the preNum and postNum, marks
   back links, marks nodes cacheable if their subtree has no back links that 
   go above the node and they have no printable or labelled descendants, and 
   makes sure no printable nodes have negative ancestors. */
int preparePattern(Node P, char pos) {
  int bh;
  CurrentPattern->numNodes++;
  if (P->preNum) {
    fatalError("The links in pattern %d form a cycle of crossing edges.",
	       CurrentPattern->num);
  }
  if (!P->names) {
    fatalError("The label \"%s\" refers to a node that has not been given a \n"
	       "pattern to match the node name.", P->label);
  }
  if (P->print) {
    if (!pos) {
      String S = printNodeToString(P);
      fatalError("Node \"%s\" is marked for printing but is part of a "
		 "non-match.", S->s);
    } else CurrentPattern->numPrints++;
  }
  P->preNum = bh = PreNum++;

  if (P->link) {
    int h = prepareLinks(P->link, pos);
    P->link->parentNode = P;
    if (h < bh) bh = h;
  }
  if (P->print || (P->label && willBePrinted(P->label))) bh = 0;
  P->cacheable = (bh >= P->preNum);
  P->postNum = PostNum++;
  return bh;
}

Pattern newPattern(void) {
  Pattern P = smartCalloc(1, sizeof(struct pattern), MEM_PATTERN);
  P->labels = hashTableCreate(8, 2, hashString, compareStrings, MEM_PATTERN);
  Patterns[NumPatterns++] = CurrentPattern = P;
  P->num = NumPatterns;
  return P;
}

void fillArraysN(Pattern R, Node P);
void fillArraysL(Pattern R, Link L) {
  int i;
  if (L->node && !L->backLink) fillArraysN(R, L->node);
  else for (i = 0; i < L->numLinks; i++)
    fillArraysL(R, L->link[i]);
}
void fillArraysN(Pattern R, Node P) {
  R->node[R->numNodes++] = P;
  if (P->print) R->print[R->numPrints++] = P;
  if (P->link) fillArraysL(R, P->link);
}

char *extractPattern(char *pattern, String S) {
  char *s, paren = FALSE;
  clearString(S);
  for (s = pattern; *s; s++) {
    if (*s == ';' && !paren) {s++; break;}
    if (*s == '"') paren = !paren;
    stringCat(S, *s);
  }
  skipBlank(&s);
  return s;
}

void buildPattern(char *pattern) {
  Pattern R;
  Node P, N;
  int maxPatterns = INIT_PATTERNS;
  char *s;
  String original = newString(256, MEM_PATTERN), 
    substituted = newString(256, MEM_PATTERN);

  if (!Patterns)
    Patterns = smartMalloc(maxPatterns * sizeof(Pattern), MEM_PATTERN);
  skipBlank(&pattern);
  while (*pattern) {
    pattern = extractPattern(pattern, original);
    macroSubstitute(original, substituted);
    s = substituted->s;
    
    if (s[0] == '@' && isspace(s[1])) {
      /* This line defines a macro. */
      s++;
      clearString(original);
      for (skipBlank(&s); !strchr(SpecialSymbols, *s); s++)
	stringCat(original, *s);
      
      clearString(Buffer);
      skipBlank(&s);
      stringAppend(Buffer, s);

      addMacro(original->s, Buffer->s);
      continue;
    }

    /* Otherwise this is actually a pattern. */
    if (NumPatterns >= maxPatterns) {
      maxPatterns *= 2;
      Patterns = smartRealloc(Patterns, maxPatterns * sizeof(Pattern), 
			      MEM_PATTERN);
    }
    R = newPattern();

    P = parsePatternNode(&s, TRUE);

    while (*s == ':') {
      s++;
      N = parsePatternNode(&s, TRUE);
      if (N && N->inLinks == 0 && N != P) {
	String S = printNodeToString(N);
	fatalError("Node \"%s\" isn't reachable from the start node in pattern %d.", S->s, CurrentPattern->num);
      }
    }
    if (*s && *s != ';')
      fatalError("Unbalanced parentheses in pattern %d", CurrentPattern->num);
    if (*s) s++;

    PreNum = PostNum = 1;
    R->numNodes = R->numPrints = 0;
    preparePattern(P, TRUE);
    R->node  = smartMalloc(R->numNodes * sizeof(Node), MEM_PATTERN);
    R->print = smartMalloc(R->numPrints * sizeof(Node), MEM_PATTERN);
    R->numNodes = R->numPrints = 0;
    fillArraysN(R, P);

    R->head = P;
  }
  freeString(original);
  freeString(substituted);
}

/* Returns the added depth. */
int printNode(Node P, int depth) {
  Wpat V;
  int i;
  /* if (P->cacheable) fputc('^', stderr); */
  if (P->not) {fputc('!', stderr); depth++;}
  for (i = 0; i < P->numNames; i++) {
    V = P->names + i;
    if (i != 0) {fputc('|', stderr); depth++;}
    if (V->regExp) {fputc('/', stderr); depth++;}
    if (V->string) {fputs(V->string, stderr); depth += strlen(V->string);}
    if (V->regExp) {fputc('/', stderr); depth++;}
  }
  if (P->label) {
    fprintf(stderr, "=%s", P->label); 
    depth += strlen(P->label) + 1;}
  return depth;
}

String printNodeToString(Node P) {
  Wpat V;
  int i;
  String S = newString(16, MEM_TEMP);
  if (P->not) stringCat(S, '!');
  for (i = 0; i < P->numNames; i++) {
    V = P->names + i;
    if (i != 0) stringCat(S, '|');
    if (V->regExp) stringCat(S, '/');
    if (V->string) stringAppend(S, V->string);
    if (V->regExp) stringCat(S, '/');
  }
  if (P->label) {
    stringCat(S, '=');
    stringAppend(S, P->label);
  }
  return S;
}

void printPattern(Node P, int depth);

void printLinks(Link L, char bracket, int depth) {
  int d = 0;
  if (!L) return;
  if (L->not) {fputc('!', stderr); d++;}
  if (L->optional) {fputc('?', stderr); d++;}
  if (L->node) {
    /* fputc(' ', stderr); d++; */
    fputs(L->ltype->code, stderr); d += strlen(L->ltype->code);
    if (L->childNum) {
      int n;
      fprintf(stderr, "%d%n", L->childNum, &n);
      d += n;
    }
    if (L->equal) {fputc('=', stderr); d++;}
    fputc(' ', stderr); d++;
    if (L->backLink) {
      fprintf(stderr, "=%s", L->node->label);
    } else printPattern(L->node, depth + d);
  } else {
    int i;
    if (bracket) {fputc('[', stderr); depth++;}
    for (i = 0; i < L->numLinks; i++) {
      if (i > 0) {
	if (L->operator == OR) {
	  fputs("| ", stderr); 
	  if (i == 1) d += 2;
	}
      }
      printLinks(L->link[i], TRUE, depth + d);
      if (i < L->numLinks - 1) {
	int j;
	fputc('\n', stderr);
	for (j = 0; j < depth; j++) fputc(' ', stderr);
      }
    }
    if (bracket) fputc(']', stderr);
  }
}

void printPattern(Node P, int depth) {
  if (P->link) {fputc('(', stderr); depth++;}
  if (P->print) {fputc('`', stderr); depth++;}
  /* if (!P->cacheable) fputc('^', stderr); */
  depth = printNode(P, depth);
  if (P->link) {
    fputc(' ', stderr);
    printLinks(P->link, FALSE, depth + 1);
    fputs(")", stderr);
  }
}

void printPatterns(void) {
  int i;
  for (i = 0; i < NumPatterns; i++) {
    printPattern(Patterns[i]->head, 0);
    fputc(';', stderr);
    fputc('\n', stderr);
  }
  fflush(stderr);
}

/*
void lookupWords(void) {
  int n, p, i;
  Wpat V;
  for (p = 0; p < NumPatterns; p++) {
    Pattern P = Patterns[p];
    for (n = 0; n < P->numNodes; n++) {
      Node N = P->node[n];
      for (i = 0; i < N->numNames; i++) {
	V = N->names + i;
	if (V->type == W_CONST) {
	  V->word = hashTableLookup(WordHash, V->string);
	  if (!V->word) V->type = W_NONE;
	}
      }
    }
  }
}
*/

/****************************** Formatted Output *****************************/

Format newFormat(void) {
  return smartCalloc(1, sizeof(struct format), MEM_FORMAT);
}

Format parseFormat(char *format) {
  char *s;
  Format F, L = NULL, A = NULL;
  String S = newString(32, MEM_TEMP);

  if (!format) return NULL;
  for (s = format; *s;) {
    F = newFormat();
    if (!A) A = F;
    else L->next = F;
    L = F;
    clearString(S);
    
    if (*s == '%') {
      s++;
      /* Unused: egoqrv */
      switch (*s) {
      case 'l': F->format = F_LONG;     s++; break;
      case 't': F->format = F_TERMINAL; s++; break;
      case 'n': F->format = F_NUM;      s++; break;
      case 'x': F->format = F_CODE;     s++; break;
      case 'u': F->format = F_TOPNODE;  s++; break;
      case 'y':	F->format = F_FIRST;  ComputeLengths = TRUE; s++; break;
      case 'z':	F->format = F_LAST;   ComputeLengths = TRUE; s++; break;
      case 'k':	F->format = F_LENGTH; ComputeLengths = TRUE; s++; break;
      case 'd':	F->format = F_DEPTH;    s++; break;
      }
      if (*s == '-' || isdigit(*s)) {
	int shift;
	sscanf(s, "%d%n", &(F->num), &shift);
	s += shift;
      }
      switch (*s) {
      case 'f':
	F->type = F_FILENAME; break;
      case 's':
	F->type = F_SNUM; break;
      case 'p':
	F->type = F_PNUM; break;
      case 'i':
	F->type = F_SMATCHNUM; break;
      case 'j':
	F->type = F_PMATCHNUM; break;
      case 'c':
	F->type = F_COMMENT; break;
      case 'h':
	F->type = F_HEAD; break;
      case 'm':
	F->type = F_MARKED; break;
      case 'w':
	F->type = F_WHOLE; break;
      case 'b':
	F->type = F_CONTEXT; 
	if (F->num <= 0) F->num = 1;
	if (F->num > NumBefore) NumBefore = F->num;
	F->num = -F->num;
	break;
      case 'a':
	F->type = F_CONTEXT;
	if (F->num <= 0) F->num = 1;
	if (F->num > NumAfter) NumAfter = F->num;
	break;
      case '=':
	F->type = F_LABEL;
	for (s++; *s && *s != '='; s++)
	  stringCat(S, *s);
	F->label = copyString(S->s, MEM_FORMAT);
	break;
      default: fatalError("Bad print format field code: %c", *s);
      }
      if (F->type == F_FILENAME || F->type == F_COMMENT) {
	clearString(S);
	if (F->num) sprintf(S->s, "%%%ds", F->num);
	else sprintf(S->s, "%%s");
      } else {
	clearString(S);
	if (F->num) sprintf(S->s, "%%%dd", F->num);
	else sprintf(S->s, "%%d");
      }
      F->string = copyString(S->s, MEM_FORMAT);
      s++;
    } else {
      F->type = F_STRING;
      for (; *s && (*s != '%' || s[1] == '%'); s++) {
	char c = *s;
	if (c == '%') s++;
	else if (c == '\\') {
	  switch (s[1]) {
	  case 'n':  c = '\n'; break;
	  case 't':  c = '\t'; break;
	  case 'b':  c = '\b'; break;
	  case 'r':  c = '\r'; break;
	  case 'f':  c = '\f'; break;
	  case 'a':  c = '\a'; break;
	  case 'v':  c = '\v'; break;
	  case '\'': c = '\''; break;
	  case '"':  c = '"';  break;
	  case '\\': c = '\\'; break;
	  default: c = s[1];
	  }
	  s++;
	}
	stringCat(S, c);
      }
      F->string = copyString(S->s, MEM_FORMAT);
    }
  }
  freeString(S);
  return A;
}

void formattedOutput(Format F) {
  Node P;
  if (!F) return;
  switch (F->type) {
  case F_STRING:
    fputs(F->string, stdout);
    break;
  case F_FILENAME:
    printf(F->string, CorpusFile);
    break;
  case F_SNUM:
    printf(F->string, SentenceNum);
    break;
  case F_PNUM:
    if (CurrentPattern) printf(F->string, CurrentPattern->num);
    else printf(F->string, 0);
    break;
  case F_SMATCHNUM:
    printf(F->string, SMatchNum);
    break;
  case F_PMATCHNUM:
    if (CurrentPattern)
      printf(F->string, CurrentPattern->matchNum);
    else printf(F->string, TMatchNum);
    break;
  case F_COMMENT:
    if (CurrentSentence->comment) printf(F->string, CurrentSentence->comment);
    break;
  case F_HEAD:
    if (CurrentPattern) printNodeTree(CurrentPattern->head, F);
    else printTree(NULL, F);
    break;
  case F_MARKED:
    printMarkedTrees(F);
    break;
  case F_WHOLE:
    printTree(Top, F);
    break;
  case F_LABEL:
    P = lookupLabel(F->label);
    /*
    if (!P) fatalError("A node labeled \"%s\" is in the output format but "
		       "doesn't exist in the\ncurrent pattern.", F->label);
    */
    printNodeTree(P, F);
    break;
  case F_CONTEXT:
    if (F->num < -NumBefore || F->num > NumAfter) printTree(NULL, F);
    else printTree(Sent[SENTENCE(F->num)]->tree[0], F);
    break;
  }
  formattedOutput(F->next);
}


/*************************** General Pattern Matching ************************/

void clearAllMatches(void) {
  Pattern R = CurrentPattern;
  int i;
  for (i = 0; i < R->numNodes; i++)
    R->node[i]->tree = NULL;
}

void clearMatches(Node P);
void clearLinkMatches(Link L) {
  if (!L || L->backLink) return;
  if (L->node) clearMatches(L->node);
  else {
    int i;
    for (i = 0; i < L->numLinks; i++)
      clearLinkMatches(L->link[i]);
  }
}

/* This removes all match info.  If a node is not matched, it is assumed that
   its linked nodes are not either.  If a node is cacheable, it can't have any
   printable descendants, so there's no need to clear old matches. */
void clearMatches(Node P) {
  if (!P || !P->tree || P->cacheable) return;
  P->tree = NULL;
  clearLinkMatches(P->link);
}


/* Does its own caching for optimization. */
char wordsMatch(Node P, Tree T) {
  Wpat V;
  Word W = T->word;
  int i, result;
  
  if (W->match[P->num] != -1) return W->match[P->num];
  for (i = 0, result = FALSE; i < P->numNames && !result; i++) {
    V = P->names + i;
    switch (V->type) {
    case W_ALL: result = TRUE; break;
    case W_NONE: break;
    case W_CONST: /* if (V->word == W) result = TRUE; break; */
      if (IgnoreCase) {
	if (!strcasecmp(V->string, W->name)) result = TRUE; break;
      } else {
	if (!strcmp(V->string, W->name)) result = TRUE; break;
      }
    case W_REGEXP: if (regExpMatch(V->regExp, W->name)) result = TRUE; break;
    }
  }
  if (P->not) result = !result;
  W->match[P->num] = result;
  return result;
}


char match_EQUAL(Link L, Tree T);
char check_EQUAL(Link L, Tree T);

char matchLink(Link L, Tree T) {
  int i;
  char match;
  if (!L) return TRUE;
  else if (L->node) {
    if (L->backLink) {
      match = (L->equal && check_EQUAL(L, T)) ? TRUE : L->ltype->check(L, T);
    } else {
      match = (L->equal && match_EQUAL(L, T)) ? TRUE : L->ltype->match(L, T);
    }
  } else if (L->operator == AND) {
    for (i = 0, match = TRUE; i < L->numLinks && match; i++)
      if (!matchLink(L->link[i], T)) match = FALSE;
  } else {
    for (i = 0, match = FALSE; i < L->numLinks && !match; i++)
      if (matchLink(L->link[i], T)) match = TRUE;
  }
  if (L->optional) return TRUE;
  return (L->not) ? !match : match;
}

/* Does its own caching for optimization. */
char matchPattern(Node P, Tree T) {
  char match = FALSE;
  if (P->cacheable && T->match[P->num] != -1) return T->match[P->num];
  if (wordsMatch(P, T)) {
    P->tree = T;
    match = matchLink(P->link, T);
    if (!match) clearMatches(P);
  }
  T->match[P->num] = match;
  return match;			  
}

#ifdef JUNK
/* This was the original way of doing it that wasn't as flexible. */

/* This tries to start the head of a pattern at every point in the tree. */
/* Returns TRUE if match was made and not finding all matches. */
char matchFullPattern(Node P, Sentence S) {
  int i;
  for (i = 0; i < S->numTrees; i++) {
    Tree T = S->tree[i];
    if (matchPattern(P, T)) {
      if (FilterMode) return TRUE;
      if (SMatchNum == 0) {
	formattedOutput(SentenceFormat);
	if (!MatchFormat && PrintComments) printComment(CurrentSentence);
      }
      SMatchNum++; PMatchNum++;
      if (MatchFormat) formattedOutput(MatchFormat);
      else {
	if (WholeSentence) printTree(Top, DefaultFormat);
	else printMarkedTrees(DefaultFormat);
	putchar('\n');
      }
      clearAllMatches();
      if (MatchMode != M_ALL) return TRUE;
    }
  }
  return FALSE;
}

/* This runs through all of the patterns and tries each one. */
void matchPatterns() {
  char done;
  int i;

  Top = CurrentSentence->tree[0];
  SMatchNum = PMatchNum = 0;
  CurrentPattern = NULL;
  formattedOutput(SentBeginFormat);
  for (i = 0, done = FALSE; i < NumPatterns && !done; i++) {
    CurrentPattern = Patterns[i];
    PMatchNum = 0;
    done = matchFullPattern(CurrentPattern->head, CurrentSentence);
    if (!FilterMode && MatchMode == M_ONE_EACH) done = FALSE;
  }
  if (FilterMode && !done) {
    if (SentenceFormat) formattedOutput(SentenceFormat);
    else {
      printTree(Top, DefaultFormat);
      putchar('\n');
    }
  }
}
#endif /* JUNK */

/* This runs through each subtree and tries all the patterns on the subtree. 
   If TMatchMode is M_FILTER, it only prints sentences with no matches.
   Otherwise:

   TMM   PMM   Behavior
   ----------------------------
   FIRST FIRST <= 1 match per sentence.  The first node that matches a pattern.
   FIRST ALL   The first matching node for each pattern. (default)
   ALL   FIRST All nodes that match one or more patterns (<= 1 match per node).
   ALL   ALL   All possible matches.
*/
void matchPatterns(void) {
  int t, p;
  char done;
  Sentence S = CurrentSentence;

  Top = S->tree[0];
  SMatchNum = 0;

  CurrentPattern = NULL;
  formattedOutput(SentBeginFormat);

  for (p = 0; p < NumPatterns; p++)
    Patterns[p]->matchNum = 0;
  for (t = 0, done = FALSE; !done && t < S->numTrees; t++) {
    Tree T = S->tree[t];
    for (p = 0; !done && p < NumPatterns; p++) {
      Pattern P = CurrentPattern = Patterns[p];
      if (TMatchMode == M_FIRST && P->matchNum > 0) continue;
      if (matchPattern(P->head, T)) {
	SMatchNum++; P->matchNum++;
        if (TMatchMode == M_FILTER) {
	  done = TRUE;
	  break;
	}
	if (SMatchNum == 1) {
	  formattedOutput(SentenceFormat);
	  if (!MatchFormat && PrintComments) printComment(CurrentSentence);
	}
	if (MatchFormat) formattedOutput(MatchFormat);
	else {
	  if (WholeSentence) printTree(Top, DefaultFormat);
	  else printMarkedTrees(DefaultFormat);
	  putchar('\n');
	}
	clearAllMatches();

	if (PMatchMode == M_FIRST) {
	  if (TMatchMode == M_FIRST && SMatchNum) done = TRUE;
	  else if (TMatchMode == M_ALL) break;
	} else /* PMatchMode == M_ALL */ {
	  if (TMatchMode == M_FIRST && SMatchNum == NumPatterns) done = TRUE;
	  /* If M_ALL/M_ALL, just keep going. */
	}
      }
    }
  }
  if (TMatchMode == M_FILTER && !SMatchNum) {
    if (SentenceFormat) formattedOutput(SentenceFormat);
    else {
      printTree(Top, DefaultFormat);
      putchar('\n');
    }
  }
}


/*************************** Link-Specific Matching **************************/

char match_DOM_LEFT(Link L, Tree T) {
  Node P = L->node;
  while (T->numKids) {
    T = T->kid[0];
    if (matchPattern(P, T)) return TRUE;
  }
  return FALSE;
}
char check_DOM_LEFT(Link L, Tree T) {
  Tree B = L->node->tree;
  if (T->preNum > B->preNum || T->postNum < B->postNum) return FALSE;
  while (T->numKids) {
    T = T->kid[0];
    if (T == B) return TRUE;
  }
  return FALSE;
}

char match_IS_DOM_LEFT(Link L, Tree T) {
  Node P = L->node;
  Tree M;
  for (M = T->parent; M && T->kidNum == 0; T = M, M = T->parent)
    if (matchPattern(P, M)) return TRUE;
  return FALSE;
}
char check_IS_DOM_LEFT(Link L, Tree T) {
  Tree B = L->node->tree;
  Tree M;
  if (B->preNum > T->preNum || B->postNum < T->postNum) return FALSE;
  for (M = T->parent; M && T->kidNum == 0; T = M, M = T->parent)
    if (M == B) return TRUE;
  return FALSE;
}

char match_DOM_RIGHT(Link L, Tree T) {
  Node P = L->node;
  while (T->numKids) {
    T = T->kid[T->numKids - 1];
    if (matchPattern(P, T)) return TRUE;
  }
  return FALSE;
}
char check_DOM_RIGHT(Link L, Tree T) {
  Tree B = L->node->tree;
  if (T->preNum > B->preNum || T->postNum < B->postNum) return FALSE;
  while (T->numKids) {
    T = T->kid[T->numKids - 1];
    if (T == B) return TRUE;
  }
  return FALSE;
}

char match_IS_DOM_RIGHT(Link L, Tree T) {
  Node P = L->node;
  Tree M;
  for (M = T->parent; M && T->kidNum == M->numKids - 1; T = M, M = T->parent)
    if (matchPattern(P, M)) return TRUE;
  return FALSE;
}
char check_IS_DOM_RIGHT(Link L, Tree T) {
  Tree B = L->node->tree;
  Tree M;
  if (B->preNum > T->preNum || B->postNum < T->postNum) return FALSE;
  for (M = T->parent; M && T->kidNum == M->numKids - 1; T = M, M = T->parent)
    if (M == B) return TRUE;
  return FALSE;
}

char match_DOM_ONLY(Link L, Tree T) {
  Node P = L->node;
  while (T->numKids == 1) {
    T = T->kid[0];
    if (matchPattern(P, T)) return TRUE;
  }
  return FALSE;
}
char check_DOM_ONLY(Link L, Tree T) {
  Tree B = L->node->tree;
  if (T->preNum > B->preNum || T->postNum < B->postNum) return FALSE;
  while (T->numKids == 1) {
    T = T->kid[0];
    if (T == B) return TRUE;
  }
  return FALSE;
}

char match_IS_DOM_ONLY(Link L, Tree T) {
  Node P = L->node;
  Tree M;
  for (M = T->parent; M && M->numKids == 1 && T->kidNum == 0; 
       T = M, M = T->parent)
    if (matchPattern(P, M)) return TRUE;
  return FALSE;
}
char check_IS_DOM_ONLY(Link L, Tree T) {
  Tree B = L->node->tree;
  Tree M;
  if (B->preNum > T->preNum || B->postNum < T->postNum) return FALSE;
  for (M = T->parent; M && M->numKids == 1 && T->kidNum == 0; 
       T = M, M = T->parent)
    if (M == B) return TRUE;
  return FALSE;
}

char match_DOMINATES(Link L, Tree T) {
  Sentence S = CurrentSentence;
  Node P = L->node;
  int i;
  for (i = T->preNum; i < S->numTrees && S->tree[i]->postNum < T->postNum; i++)
    if (matchPattern(P, S->tree[i])) return TRUE;
  return FALSE;
}
char check_DOMINATES(Link L, Tree T) {
  Tree B = L->node->tree;
  return (B->preNum > T->preNum && B->postNum < T->postNum);
}

char match_IS_DOMINATED(Link L, Tree T) {
  Node P = L->node;
  for (T = T->parent; T; T = T->parent)
    if (matchPattern(P, T)) return TRUE;
  return FALSE;
}
char check_IS_DOMINATED(Link L, Tree T) {
  Tree B = L->node->tree;
  return (T->preNum > B->preNum && T->postNum < B->postNum);
}

char match_HAS_CHILD(Link L, Tree T) {
  Node P = L->node;
  int i;
  for (i = 0; i < T->numKids && !Done; i++)
    if (matchPattern(P, T->kid[i])) return TRUE;
  return FALSE;
}
char check_HAS_CHILD(Link L, Tree T) {
  Tree B = L->node->tree;
  return (B->parent == T);
}

char match_HAS_CHILD_NUM(Link L, Tree T) {
  int i = L->childNum;
  if (i < 0) i += T->numKids; else i--;
  if (i < 0 || i >= T->numKids) return FALSE;
  return matchPattern(L->node, T->kid[i]);
}
char check_HAS_CHILD_NUM(Link L, Tree T) {
  Tree B = L->node->tree;
  int i = L->childNum;
  if (i < 0) i += T->numKids; else i--;
  return (B->parent == T && B->kidNum == i);
}

char match_HAS_CHILD_ONLY(Link L, Tree T) {
  if (T->numKids != 1) return FALSE;
  return matchPattern(L->node, T->kid[0]);
}
char check_HAS_CHILD_ONLY(Link L, Tree T) {
  Tree B = L->node->tree;
  return (B->parent == T && T->numKids == 1);
}

char match_IS_CHILD(Link L, Tree T) {
  Node P = L->node;
  if (T->parent && matchPattern(P, T->parent)) return TRUE;
  return FALSE;
}
char check_IS_CHILD(Link L, Tree T) {
  Tree B = L->node->tree;
  return (T->parent == B);
}

char match_IS_CHILD_NUM(Link L, Tree T) {
  int i = L->childNum;
  Tree M = T->parent;
  if (!M) return FALSE;
  if (i < 0) i += M->numKids; else i--;
  if (i != T->kidNum) return FALSE;
  return matchPattern(L->node, M);
}
char check_IS_CHILD_NUM(Link L, Tree T) {
  Tree B = L->node->tree;
  int i = L->childNum;
  if (i < 0) i += B->numKids; else i--;
  return (T->parent == B && T->kidNum == i);
}

char match_IS_CHILD_ONLY(Link L, Tree T) {
  Node P = L->node;
  if (T->parent && T->parent->numKids == 1 && 
      matchPattern(P, T->parent)) return TRUE;
  return FALSE;
}
char check_IS_CHILD_ONLY(Link L, Tree T) {
  Tree B = L->node->tree;
  return (T->parent == B && T->parent->numKids == 1);
}

char match_IS_PRIOR(Link L, Tree T) {
  Sentence S = CurrentSentence;
  Node P = L->node;
  int i;
  for (i = T->preNum; i < S->numTrees; i++)
    if (S->tree[i]->postNum > T->postNum && matchPattern(P, S->tree[i])) 
      return TRUE;
  return FALSE;
}
char check_IS_PRIOR(Link L, Tree T) {
  Tree B = L->node->tree;
  return (T->preNum < B->preNum && T->postNum < B->postNum);
}

char match_IS_AFTER(Link L, Tree T) {
  Sentence S = CurrentSentence;
  Node P = L->node;
  int i;
  for (i = T->preNum - 2; i >= 0; i--)
    if (S->tree[i]->postNum < T->postNum && matchPattern(P, S->tree[i]))
      return TRUE;
  return FALSE;
}
char check_IS_AFTER(Link L, Tree T) {
  Tree B = L->node->tree;
  return (T->preNum > B->preNum && T->postNum > B->postNum);
}

char match_IMM_PRIOR(Link L, Tree T) {
  Node P = L->node;
  while (T->parent && T->kidNum == T->parent->numKids - 1) T = T->parent;
  if (!T->parent) return FALSE;
  for (T = T->parent->kid[T->kidNum + 1]; T; T = T->kid[0]) {
    if (matchPattern(P, T)) return TRUE;
    if (!T->numKids) break;
  }
  return FALSE;
}
char check_IMM_PRIOR(Link L, Tree T) {
  Tree B = L->node->tree;
  while (T->parent && T->kidNum == T->parent->numKids - 1) T = T->parent;
  if (!T->parent) return FALSE;
  for (T = T->parent->kid[T->kidNum + 1]; T; T = T->kid[0]) {
    if (T == B) return TRUE;
    if (!T->numKids) break;
  }
  return FALSE;
}

char match_IMM_AFTER(Link L, Tree T) {
  Node P = L->node;
  while (T->parent && T->kidNum == 0) T = T->parent;
  if (!T->parent) return FALSE;
  for (T = T->parent->kid[T->kidNum - 1]; T; T = T->kid[T->numKids - 1]) {
    if (matchPattern(P, T)) return TRUE;
    if (!T->numKids) break;
  }
  return FALSE;
}
char check_IMM_AFTER(Link L, Tree T) {
  Tree B = L->node->tree;
  while (T->parent && T->kidNum == 0) T = T->parent;
  if (!T->parent) return FALSE;
  for (T = T->parent->kid[T->kidNum - 1]; T; T = T->kid[T->numKids - 1]) {
    if (T == B) return TRUE;
    if (!T->numKids) break;
  }
  return FALSE;
}

char match_SIS_PRIOR(Link L, Tree T) {
  Node P = L->node;
  int i;
  Tree M = T->parent;
  if (!M) return FALSE;
  for (i = T->kidNum + 1; i < M->numKids; i++)
    if (matchPattern(P, M->kid[i])) return TRUE;
  return FALSE;
}
char check_SIS_PRIOR(Link L, Tree T) {
  Tree B = L->node->tree;
  return (T->parent == B->parent && T->kidNum < B->kidNum);
}

char match_SIS_AFTER(Link L, Tree T) {
  Node P = L->node;
  int i;
  Tree M = T->parent;
  if (!M) return FALSE;
  for (i = T->kidNum - 1; i >= 0; i--)
    if (matchPattern(P, M->kid[i])) return TRUE;
  return FALSE;
}
char check_SIS_AFTER(Link L, Tree T) {
  Tree B = L->node->tree;
  return (T->parent == B->parent && T->kidNum > B->kidNum);
}

char match_SIS_IMM_PRIOR(Link L, Tree T) {
  Node P = L->node;
  Tree M = T->parent;
  if (!M || T->kidNum == M->numKids - 1) return FALSE;
  if (matchPattern(P, M->kid[T->kidNum + 1])) return TRUE;
  return FALSE;
}
char check_SIS_IMM_PRIOR(Link L, Tree T) {
  Tree B = L->node->tree;
  return (T->parent == B->parent && T->kidNum + 1 == B->kidNum);
}

char match_SIS_IMM_AFTER(Link L, Tree T) {
  Node P = L->node;
  Tree M = T->parent;
  if (!M || T->kidNum == 0) return FALSE;
  if (matchPattern(P, M->kid[T->kidNum - 1])) return TRUE;
  return FALSE;
}
char check_SIS_IMM_AFTER(Link L, Tree T) {
  Tree B = L->node->tree;
  return (T->parent == B->parent && T->kidNum - 1 == B->kidNum);
}

char match_HAS_SIS(Link L, Tree T) {
  Node P = L->node;
  int i;
  Tree M = T->parent;
  if (!M) return FALSE;
  for (i = 0; i < M->numKids; i++) {
    if (i == T->kidNum) continue;
    if (matchPattern(P, M->kid[i])) return TRUE;
  }
  return FALSE;
}
char check_HAS_SIS(Link L, Tree T) {
  Tree B = L->node->tree;
  return (T->parent == B->parent && T != B);
}

char match_EQUAL(Link L, Tree T) {
  return matchPattern(L->node, T);
}
char check_EQUAL(Link L, Tree T) {
  return (T == L->node->tree);
}

/* Matches any node with the same name (including self) */
char match_SAME_NAME(Link L, Tree T) {
  Sentence S = CurrentSentence;
  Node P = L->node;
  int i;
  for (i = 0; i < S->numTrees; i++)
    if (S->tree[i]->word == T->word && matchPattern(P, S->tree[i]))
      return TRUE;
  return FALSE;
}

char check_SAME_NAME(Link L, Tree T) {
  return (T->word == L->node->tree->word);
}


/********************************* Link Types ********************************/

void addLinkType(int type, char *code, int cost, char (*match)(Link, Tree),
		 char (*check)(Link, Tree)) {
  Ltype L = Ltypes + type;
  L->type  = type;
  L->code  = code;
  L->len   = strlen(code);
  L->cost  = cost;
  L->match = match;
  L->check = check;
}

/* Links must be ordered so substrings come after superstrings. */
void registerLinkTypes(void) {
  Ltypes = smartMalloc(LINK_TYPES * sizeof(struct ltype), MEM_LINK);
  addLinkType(DOM_LEFT,        "<<,", 2, 
	      match_DOM_LEFT, check_DOM_LEFT);
  addLinkType(IS_DOM_LEFT,     ">>,", 2, 
	      match_IS_DOM_LEFT, check_IS_DOM_LEFT);
  addLinkType(DOM_RIGHT,       "<<`", 2, 
	      match_DOM_RIGHT, check_DOM_RIGHT);
  addLinkType(IS_DOM_RIGHT,    ">>`", 2, 
	      match_IS_DOM_RIGHT, check_IS_DOM_RIGHT);
  addLinkType(DOM_ONLY,        "<<:", 2, 
	      match_DOM_ONLY, check_DOM_ONLY);
  addLinkType(IS_DOM_ONLY,     ">>:", 2, 
	      match_IS_DOM_ONLY, check_IS_DOM_ONLY);
  addLinkType(DOMINATES,       "<<",  3, 
	      match_DOMINATES, check_DOMINATES);
  addLinkType(IS_DOMINATED,    ">>",  2, 
	      match_IS_DOMINATED, check_IS_DOMINATED);
  addLinkType(HAS_CHILD_LEFT,  "<,",  1, 
	      match_HAS_CHILD_NUM, check_HAS_CHILD_NUM);
  addLinkType(HAS_CHILD_RIGHT, "<`",  1, 
	      match_HAS_CHILD_NUM, check_HAS_CHILD_NUM);
  addLinkType(HAS_CHILD_ONLY,  "<:",  1, 
	      match_HAS_CHILD_ONLY, check_HAS_CHILD_ONLY);
  addLinkType(HAS_CHILD,       "<",   2, 
	      match_HAS_CHILD, check_HAS_CHILD);
  addLinkType(IS_CHILD_LEFT,   ">,",  1, 
	      match_IS_CHILD_NUM, check_IS_CHILD_NUM);
  addLinkType(IS_CHILD_RIGHT,  ">`",  1, 
	      match_IS_CHILD_NUM, check_IS_CHILD_NUM);
  addLinkType(IS_CHILD_ONLY,   ">:",  1, 
	      match_IS_CHILD_ONLY, check_IS_CHILD_ONLY);
  addLinkType(IS_CHILD,        ">",   1, 
	      match_IS_CHILD, check_IS_CHILD);
  addLinkType(IS_PRIOR,        "..",  4, 
	      match_IS_PRIOR, check_IS_PRIOR);
  addLinkType(IS_AFTER,        ",,",  4, 
	      match_IS_AFTER, check_IS_AFTER);
  addLinkType(IMM_PRIOR,       ".",   2, 
	      match_IMM_PRIOR, check_IMM_PRIOR);
  addLinkType(IMM_AFTER,       ",",   2, 
	      match_IMM_AFTER, check_IMM_AFTER);
  addLinkType(SIS_PRIOR,       "$..", 2, 
	      match_SIS_PRIOR, check_SIS_PRIOR);
  addLinkType(SIS_AFTER,       "$,,", 2, 
	      match_SIS_AFTER, check_SIS_AFTER);
  addLinkType(SIS_IMM_PRIOR,   "$.",  1, 
	      match_SIS_IMM_PRIOR, check_SIS_IMM_PRIOR);
  addLinkType(SIS_IMM_AFTER,   "$,",  1, 
	      match_SIS_IMM_AFTER, check_SIS_IMM_AFTER);
  addLinkType(HAS_SIS,         "$",   2, 
	      match_HAS_SIS, check_HAS_SIS);
  addLinkType(HAS_CHILD_NUM,   "<",   1, 
	      match_HAS_CHILD_NUM, check_HAS_CHILD_NUM);
  addLinkType(IS_CHILD_NUM,    ">",   1, 
	      match_IS_CHILD_NUM, check_IS_CHILD_NUM);
  addLinkType(EQUAL,           "=",   1, 
	      match_EQUAL, check_EQUAL);
  addLinkType(SAME_NAME,       "~",   5, 
	      match_SAME_NAME, check_SAME_NAME);
}


/****************************** Preparing Corpora ****************************/

void addKid(Tree K, Tree T) {
  K->parent = T;
  K->kidNum = T->numKids++;
  if (T->numKids > T->maxKids) {
    T->maxKids *= 2;
    T->kid = smartRealloc(T->kid, T->maxKids * sizeof(Tree), MEM_TREE);
  }
  T->kid[T->numKids - 1] = K;
}

Tree addTree(Sentence S) {
  Tree T = getTree();
  if (S->maxTrees <= S->numTrees) {
    S->maxTrees *= 2;
    S->tree = smartRealloc(S->tree, S->maxTrees * sizeof(Tree), MEM_SENTENCE);
  }
  S->tree[S->numTrees++] = T;
  T->parent = NULL;
  T->numKids = 0;
  T->word = NULL;
  return T;
}

void setName(Tree T, Sentence S, char *word) {
  Tree N;
  Word W = hashTableLookup(WordHash, word);
  if (!W) fatalError("Unknown word or symbol: %s\n", word);
  if (!T->word) T->word = W;
  else {
    N = addTree(S);
    addKid(N, T);
    N->word = W;
  }
}

char parseTree(FILE *file, Sentence S) {
  String word = newString(64, MEM_TEMP);
  String comment = newString(64, MEM_TEMP);
  char inWord = FALSE;
  int c, depth = 0;
  Tree T = NULL, N;
  
  S->numTrees = 0;
  smartFree(S->comment, MEM_SENTENCE);
  S->comment = NULL;

  do {c = fgetc(file);} while (isspace(c));
  while (c == '#') {
    clearString(comment);
    while (c == '#') {
      if (comment->numChars > 0) stringCat(comment, '\n');
      /* do {c = fgetc(file);} while (isspace(c) && c != '\n'); */
      while (c != '\n' && c != EOF) {
	stringCat(comment, c);
	c = fgetc(file);
      }
      if (c == EOF) return 0;
      c = fgetc(file);
    }
    while (isspace(c)) c = fgetc(file);
  }
  if (c == EOF) return 0;
  if (c != '(') 
    fatalError("Tree %d doesn't start with an open parenthesis.", 
	       SentenceNum + 1);
  if (comment->numChars > 0) 
    S->comment = copyString(comment->s, MEM_SENTENCE);
  
  for (; c != EOF; c = fgetc(file)) {
    if (c == '(') {
      if (inWord) {
	setName(T, S, word->s);
	inWord = FALSE;
      }
      N = addTree(S);
      if (T) {
	if (!T->word) fatalError("Tree %d has a symbol with no name.  "
				 "That's not allowed.", SentenceNum + 1);
	addKid(N, T);
      }
      T = N;
      depth++;
    } else if (c == ')') {
      if (inWord) {
	setName(T, S, word->s);
	inWord = FALSE;
      } else if (!T->word) {
        fatalError("Tree %d has a node with no symbol name.  "
                   "That's not allowed.", SentenceNum + 1);
      }
      T = T->parent;
      depth--;
      if (depth == 0) break;
    } else if (isspace(c)) {
      if (inWord) {
	setName(T, S, word->s);
	inWord = FALSE;
      }
    } else {
      if (!inWord) clearString(word);
      stringCat(word, c);
      inWord = TRUE;
    }
  }
  freeString(word);
  return 1;
}

void loadWords(FILE *file) {
  String S = newString(32, MEM_TEMP);
  int c, depth = 0;
  char inWord = FALSE;
  NumSentences = 0;
  while ((c = fgetc(file)) != EOF) {
    if (isspace(c) || c == '(' || c == ')') {
      if (inWord) {
	if (!hashTableLookup(WordHash, S->s)) {
	  Word W = smartCalloc(1, sizeof(struct word), MEM_WORD);
	  W->name = copyString(S->s, MEM_WORD);
	  hashTableInsert(WordHash, W->name, W, FALSE);
	}
	inWord = FALSE;
      }
      if (c == '(') depth++;
      else if (c == ')') {
	depth--;
	if (depth == 0) NumSentences++;
      }
    } else {
      if (!inWord) {
	clearString(S);
	inWord = TRUE;
      }
      stringCat(S, c);
    }
  }
  freeString(S);
}

char writeWord(HashTable T, void *key, void *data, void *junk) {
  WordList[NumWords++] = (Word) data;
  return 0;
}

int compareWords(const void *a, const void *b) {
  Word A = *((Word *) a);
  Word B = *((Word *) b);
  return strcmp(A->name, B->name);
}

void writeWords(FILE *out) {
  int i, j;
  Word W, P = NULL;
  WordList = smartMalloc(hashTableNumEntries(WordHash) * sizeof(Word), 
			 MEM_WORD);
  hashTableForEach(WordHash, writeWord, NULL);
  qsort(WordList, NumWords, sizeof(Word), compareWords);
  
  writeBinInt(out, NumWords);
  for (i = 0; i < NumWords; i++) {
    W = WordList[i];
    W->num = i;
    if (P) {
      for (j = 0; P->name[j] == W->name[j]; j++);
    } else j = 0;
    writeBinChar(out, j);
    writeBinString(out, WordList[i]->name + j, -1);
    P = W;
  }
}

void prepareCorpus(char *infile, char *outfile) {
  int i;
  FILE *file, *out;
  Sentence S = newSentence();
  file = fatalReadFile(infile);
  out = fatalWriteFile(outfile, FALSE);

  WordHash = hashTableCreate(INIT_HASH, 2, hashString, compareStrings, 
			     MEM_WORD);

  /* Write the header: */
  writeBinString(out, COOKIE2, -1);
  if (PrintComments) writeBinChar(out, 'C');
  if (ManyKids) writeBinChar(out, 'K');
  writeBinChar(out, '\0');

  loadWords(file);
  writeWords(out);

  closeFile(file);
  file = fatalReadFile(infile);

  writeBinInt(out, NumSentences);
  reportProgress(NumSentences, 4 * NumSentences);
  for (SentenceNum = 0; parseTree(file, S); SentenceNum++) {
    if (PrintComments) writeBinString(out, S->comment, -1);
    writeBinShort(out, S->numTrees);
    for (i = 0; i < S->numTrees; i++) {
      Tree T = S->tree[i];
      writeBinInt(out, T->word->num);
      if (ManyKids) 
	writeBinShort(out, T->numKids);
      else {
	if (T->numKids >= 256)
	  fatalError("A parse tree node has %d kids.  You must use the -K flag"
		     "\nwhen preparing this corpus.", T->numKids);
	writeBinChar(out, (unsigned char) T->numKids);
      }
    }
    returnSentence(S);
    reportProgress(3 * SentenceNum + NumSentences, 4 * NumSentences);
  }

  hashTableFree(WordHash);
  for (i = 0; i < NumWords; i++) {
    smartFree(WordList[i]->name, MEM_WORD);
    smartFree(WordList[i], MEM_WORD);
  }
  smartFree(WordList, MEM_WORD);
  freeSentence(S);
  NumWords = SentenceNum = 0;
  closeFile(file);
  closeFile(out);
}


/************************** Major Control Functions **************************/

void performMatch(void) {
  CurrentSentence = Sent[SENTENCE(0)];
  matchPatterns();
  reportProgress(SentenceNum, NumSentences);
  SentenceNum++;
}

/* SMatchNum and TMatchNum and globals because they may be set by one
   call to this and used in the next one. */
void performExtractions(FILE *file) {
  char first = TRUE;
  int val = 2;
  formattedOutput(SentBeginFormat);
  if (SMatchNum == 0)
    val = fscanf(file, " %d:%d", &SMatchNum, &TMatchNum);
  if (val == 2 && SMatchNum <= SentenceNum) {
    CurrentSentence = Sent[SENTENCE(0)];
    Top = CurrentSentence->tree[0];
    while (SMatchNum == SentenceNum && val == 2) {
      if (TMatchNum <= 0 || TMatchNum > CurrentSentence->numTrees)
	fatalError("Node %d doesn't exist in sentence %d\n", 
		   TMatchNum, SMatchNum);
      CurrentPattern->head->tree = 
	CurrentSentence->tree[TMatchNum - 1];
      
      if (first) {
	formattedOutput(SentenceFormat);
	first = FALSE;
      }
      if (MatchFormat) formattedOutput(MatchFormat);
      else {
	if (WholeSentence) printTree(Top, DefaultFormat);
	else printTree(CurrentPattern->head->tree, DefaultFormat);
	putchar('\n');
      }
      val = fscanf(file, " %d:%d", &SMatchNum, &TMatchNum);
    }
    if (SMatchNum < SentenceNum)
      fatalError("The subtree codes are not in increasing sentence order.");
  }
  if (val == 0 || val == EOF) {
    closeFile(file);
    exit(0);
  } else if (val != 2)
    fatalError("The subtree code file was badly formatted or truncated.");
  reportProgress(SentenceNum, NumSentences);
  SentenceNum++;
  return;
}


/******************************* Usage Summaries *****************************/

int usage(char *progName) {
  debug("                            TGrep2 version %s\n\n", VERSION);
  debug("Usage: %s [options] <pattern1>...\n", progName);
  debug("\nOPTIONS:\n");
  debug("  -c <corpus>  sets the corpus file (else taken from TGREP2_CORPUS env. var.)\n");
  debug("  -a           all matching subtrees are reported, not just the first\n");
  debug("  -f           only the first subtree matching each pattern is reported\n");
  debug("  -v           only non-matching sentences are reported\n");
  debug("  -i           ignores case on all node-name matches\n");
  debug("  -b <format>  the print format used at the start of every sentence\n");
  debug("  -s <format>  the print format used on the first match to each sentence\n");
  debug("  -m <format>  the print format used on each match\n");
  debug("  -l           if there is no -m, trees are printed in long format\n");
  debug("  -t           if there is no -m, only terminals of trees are printed\n");
  debug("  -x           if there is no -m, sentence and node numbers (s:n) are printed\n");
  debug("  -w           if there is no -m, the whole sentence is printed on a match\n");
  debug("  -d           prevents the reordering of links for greater efficiency\n");
  debug("  -r <seconds> prints the completion percentage to stderr this often\n");
  debug("  -e           In place of a pattern, a file is given which specifies the\n"
	"               sentences and node numbers (s:n) to extract\n");
  debug("  -p <text-file> <corpus>  Converts a text file to a binary corpus file.\n");
  debug("  -z           pretty-prints the pattern (for debugging patterns)\n");
  debug("  -C           sentence comments will be stored when writing a corpus with\n"
	"               -p or printed on a match when searching.\n");
  debug("  -K           if any parse tree node has more than 255 kids, you must\n"
	"               use this flag when building the corpus.\n");
  debug("  -h           prints this message and a list of the link codes.\n");
  debug("\nOUTPUT FORMAT FIELDS:\n");
  debug("Non-trees:\n");
  debug("  %%f           the corpus name\n");
  debug("  %%s           the sentence number\n");
  debug("  %%p           the number of the matching pattern\n");
  debug("  %%i           the match number on the current sentence\n");
  debug("  %%j           the match number on the current sentence using this pattern\n");
  debug("  %%c           the sentence comment, if there is one\n");
  debug("  Immediately following the %% can be a positive or negative number "
	"specifying\n  the format width, as with printf.\n");
  debug("Trees:\n");
  debug("  %%h           the tree matched by the head (first) node of the pattern\n");
  debug("  %%m           all marked nodes, or the head node if none are marked\n");
  debug("  %%w           the top node in the sentence tree\n");
  debug("  %%=foo=       the subtree matched by the node labeled foo\n");
  debug("  %%Nb          the sentence N before the current one (N should be positive)\n");
  debug("  %%Na          the sentence N after the current one (N should be positive)\n");
  debug("Tree Printing Styles:\n");
  debug("  By default, trees are printed in compact, single-line format.\n");
  debug("  Alternately, one of these formatting codes can immediately follow the %:\n");
  debug("  %%l...        prints the tree in long format (multi-line, indented)\n");
  debug("  %%t...        prints the terminal symbols only\n");
  debug("  %%u...        prints only the name of the top node in the tree\n");
  debug("  %%n...        prints the depth-first pre-order node number of the tree head\n");
  debug("  %%x...        prints the sentence num and node num separated by a colon\n");
  debug("  %%k...        prints the length of the tree in terminals\n");
  debug("  %%d...        prints the depth of the tree (a terminal has depth 1)\n");
  debug("  %%y...        prints the terminal index of the first terminal in the tree\n");
  debug("  %%z...        prints the terminal index of the last terminal in the tree\n");
  debug("\n");
  return 1;
}

int linkSummary(void) {
  debug("LINK CODES:\n");
  debug("  A < B       A is the parent of (immediately dominates) B.\n");
  debug("  A > B       A is the child of B.\n");
  debug("  A <N B      B is the Nth child of A (the first child is <1).\n");
  debug("  A >N B      A is the Nth child of B (the first child is >1).\n");
  debug("  A <, B      Synonymous with A <1 B.\n");
  debug("  A >, B      Synonymous with A >1 B.\n");
  debug("  A <-N B     B is the Nth-to-last child of A (the last child is <-1).\n");
  debug("  A >-N B     A is the Nth-to-last child of B (the last child is >-1).\n");
  debug("  A <- B      B is the last child of A (synonymous with A <-1 B).\n");
  debug("  A >- B      A is the last child of B (synonymous with A >-1 B).\n");
  debug("  A <` B      B is the last child of A (also synonymous with A <-1 B).\n");
  debug("  A >` B      A is the last child of B (also synonymous with A >-1 B).\n");
  debug("  A <: B      B is the only child of A.\n");
  debug("  A >: B      A is the only child of B.\n");
  debug("  A << B      A dominates B (A is an ancestor of B).\n");
  debug("  A >> B      A is dominated by B (A is a descendant of B).\n");
  debug("  A <<, B     B is a left-most descendant of A.\n");
  debug("  A >>, B     A is a left-most descendant of B.\n");
  debug("  A <<` B     B is a right-most descendant of A.\n");
  debug("  A >>` B     A is a right-most descendant of B.\n");
  debug("  A <<: B     There is a single path of descent from A and B is on it.\n");
  debug("  A >>: B     There is a single path of descent from B and A is on it.\n");
  debug("  A . B       A immediately precedes B.\n");
  debug("  A , B       A immediately follows B.\n");
  debug("  A .. B      A precedes B.\n");
  debug("  A ,, B      A follows B.\n");
  debug("  A $ B       A is a sister of B (and A != B).\n");
  debug("  A $. B      A is a sister of and immediately precedes B.\n");
  debug("  A $, B      A is a sister of and immediately follows B.\n");
  debug("  A $.. B     A is a sister of and precedes B.\n");
  debug("  A $,, B     A is a sister of and follows B.\n");
  debug("  A = B       A is also matched by B.\n");
  debug("\n");
  return 0;
}


/************************************* Main **********************************/

int main(int argc, char *argv[]) {
  int i, opt;
  extern char *optarg;
  extern int optind;
  char *sformat = NULL, *mformat = NULL, *bformat = NULL, 
    checkPattern = FALSE, extractionMode = FALSE, corpusPrepared = FALSE;
  String pattern = newString(256, MEM_PATTERN);
  FILE *corpus = NULL, *codeFile = NULL;

  /*
  smartAllocInit(MEM_TYPES, 2048);
  smartAllocRegister(MEM_TEMP,    "Temp");
  smartAllocRegister(MEM_LINK,    "Link");
  smartAllocRegister(MEM_TREE,   "Tree");
  smartAllocRegister(MEM_WORD,    "Word");
  smartAllocRegister(MEM_STRING,  "String");
  smartAllocRegister(MEM_PATTERN, "Pattern");
  smartAllocRegister(MEM_FORMAT,  "Format");
  */

  while ((opt = getopt(argc, argv, "?ab:c:CdefhiKlm:p:r:s:tuvwxz")) != -1) {
    switch (opt) {
    case 'a':
      TMatchMode = M_ALL;
      break;
    case 'b':
      bformat = optarg;
      break;
    case 'c':
      CorpusFile = optarg;
      break;
    case 'C':
      PrintComments = TRUE;
      break;
    case 'd':
      ReorderLinks = FALSE;
      break;
    case 'e':
      extractionMode = TRUE;
      break;
    case 'f':
      PMatchMode = M_FIRST;
      break;
    case '?':
    case 'h':
      usage(argv[0]);
      linkSummary();
      return 0;
    case 'i':
      IgnoreCase = TRUE;
      break;
    case 'K':
      ManyKids = TRUE;
      break;
    case 'l':
      DefFormat = F_LONG;
      break;
    case 'm':
      mformat = optarg;
      break;
    case 'p':
      prepareCorpus(optarg, argv[optind]);
      corpusPrepared = TRUE;
      CorpusFile = argv[optind++];
      break;
    case 'r':
      ReportInterval = atoi(optarg);
      break;
    case 's':
      sformat = optarg;
      break;
    case 't':
      DefFormat = F_TERMINAL;
      break;
    case 'u':
      DefFormat = F_TOPNODE;
      break;
    case 'v':
      TMatchMode = M_FILTER;
      PMatchMode = M_FIRST;
      break;
    case 'w':
      WholeSentence = TRUE;
      break;
    case 'x':
      DefFormat = F_CODE;
      break;
    case 'z':
      checkPattern = TRUE;
      break;
    default: 
      error("Unrecognized option: -%c", opt);
      return usage(argv[0]);
    }
  }

  /* If a corpus was built and there is no pattern, exit. */
  if (corpusPrepared && optind == argc) return 0;
  /* If there is no pattern, print the usage. */
  if (optind == argc) {
    error("A pattern must be specified.");
    return usage(argv[0]);
  }

  registerLinkTypes();

  /* Build the output formats. */
  SentBeginFormat = parseFormat(bformat);
  SentenceFormat = parseFormat(sformat);
  MatchFormat = parseFormat(mformat);
  DefaultFormat = newFormat();
  DefaultFormat->type = F_MARKED;
  DefaultFormat->format = DefFormat;

  /* Build the structures to hold the current and context sentences. */
  TotalStored = NumBefore + NumAfter + 1;
  Sent = smartMalloc(TotalStored * sizeof(Sentence), MEM_SENTENCE);
  for (i = 0; i < TotalStored; i++)
    Sent[i] = newSentence();
  
  Buffer = newString(INIT_BUFFER, MEM_STRING);
  /* If -e was used, the pattern is actually the name of a file of codes. */
  if (extractionMode) {
    if (optind != argc - 1)
      fatalError("Only one code file may be given in extraction mode (-e).");
    codeFile = fatalReadFile(argv[optind]);
    /* Build a dummy pattern. */
    Patterns = smartMalloc(sizeof(Pattern), MEM_PATTERN);
    newPattern();
    CurrentPattern->head = newNode();
  } else {
    for (i = optind; i < argc; i++) {
      clearString(pattern);
      /* If the pattern is the name of a file, load it into a string. */
      if (!loadPatternFile(argv[i], pattern))
	stringAppend(pattern, argv[i]);

      /* Parse and compile the patterns. */
      buildPattern(pattern->s);
      if (checkPattern) printPatterns();
    }
    if (NumPatterns == 0) fatalError("No patterns were specified.");
  }

  /* Open the corpus file and load the words. */
  if (!CorpusFile) CorpusFile = getenv("TGREP2_CORPUS");
  if (!CorpusFile) 
    fatalError("A corpus file must be specified with either the -c option\n"
	       "or the TGREP2_CORPUS environment variable.");
  corpus = openCorpusFile(CorpusFile);
  /* lookupWords(); */
  
  /* This loads the sentence trees and does the matching.  It remembers 
     enough patterns before and after the current one to be able to print 
     them if requested. */
  for (i = 0, SentenceNum = 1; i < NumSentences; i++) {
    buildSentence(Sent[i % TotalStored], corpus);
    if (i >= NumAfter) {
      if (extractionMode) performExtractions(codeFile);
      else performMatch();
    }
  }
  for (i = 0; i < NumAfter; i++) {
    returnSentence(Sent[SENTENCE(NumAfter)]);
    if (extractionMode) performExtractions(codeFile);
    else performMatch();
  }
  closeFile(corpus);
  return 0;
}

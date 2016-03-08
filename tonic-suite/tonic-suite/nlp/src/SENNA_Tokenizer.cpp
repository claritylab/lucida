#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "SENNA_Tokenizer.h"
#include "SENNA_utils.h"

#define MAX_WORD_SIZE 256

static void tokenize_gazetteer(int **gazt_idx_, SENNA_Tokenizer *tokenizer,
                               SENNA_Hash *hash) {
  int i, j, k;
  int entity_size;
  int idxhash;
  char **words = tokenizer->tokens->words;
  int *gazt_idx;

  *gazt_idx_ = SENNA_realloc(*gazt_idx_, sizeof(int), tokenizer->max_tokens);
  gazt_idx = *gazt_idx_;

  for (i = 0; i < tokenizer->tokens->n; i++)
    gazt_idx[i] = tokenizer->gazt_hash_no_entity_idx;

  for (i = 0; i < tokenizer->tokens->n; i++) {
    entity_size = 0;
    for (j = 0; j < tokenizer->tokens->n - i; j++) {
      int word_size = strlen(words[i + j]);
      if (entity_size + word_size + 1 > tokenizer->max_entity_size) {
        tokenizer->entity = SENNA_realloc(tokenizer->entity, sizeof(char),
                                          entity_size + word_size + 1);
        tokenizer->max_entity_size = entity_size + word_size + 1;
      }

      if (j > 0) tokenizer->entity[entity_size - 1] = ' ';
      for (k = 0; k < word_size; k++)
        tokenizer->entity[entity_size++] = (char)tolower(words[i + j][k]);
      tokenizer->entity[entity_size++] = '\0';

      idxhash = SENNA_Hash_index(hash, tokenizer->entity);

      if (idxhash < 0) break;

      if (SENNA_Hash_is_admissible_index(hash, idxhash)) {
        for (k = 0; k <= j; k++)
          gazt_idx[i + k] = tokenizer->gazt_hash_is_entity_idx;
      }
    }
  }
}

SENNA_Tokens *SENNA_Tokenizer_tokenize(SENNA_Tokenizer *tokenizer,
                                       const char *sentence) {
  const char *sentence0n;
  int offset0n;

  /* first replace all numbers by '0' */
  {
    int sentence_size = strlen(sentence) + 1;
    if (sentence_size > tokenizer->max_sentence0n_size) {
      tokenizer->max_sentence0n_size = sentence_size;
      tokenizer->sentence0n =
          SENNA_realloc(tokenizer->sentence0n, sizeof(char), sentence_size);
      tokenizer->offset0n2raw =
          SENNA_realloc(tokenizer->offset0n2raw, sizeof(int), sentence_size);
    }
  }

  {
    const char *reader = sentence;
    char *writer = tokenizer->sentence0n;
    int reader_offset = 0;
    int writer_offset = 0;
    while (1) {
      char c = reader[reader_offset];
      int number_size = 0;

      if (isdigit(c) || c == '+' || c == '-' || c == '.' || c == ',')
        SENNA_tokenize_number(&number_size, reader + reader_offset);

      tokenizer->offset0n2raw[writer_offset] = reader_offset;
      if (number_size) {
        writer[writer_offset++] = '0';
        reader_offset += number_size;
      } else {
        writer[writer_offset++] = c;
        reader_offset++;

        if (c == '\0') break;
      }
    }
  }

  sentence0n = tokenizer->sentence0n;
  offset0n = 0;
  tokenizer->tokens->n = 0;
  while (1) {
    int sizetoken;
    int sizealphanumeric;
    int sizedictionary;
    int idxhash;
    int incsize;

    while (isspace(sentence0n[offset0n])) offset0n++;

    if (tokenizer->is_tokenized) {
      SENNA_tokenize_untilspace(&sizetoken, sentence0n + offset0n);
      SENNA_tokenize_dictionarymatch(&sizedictionary, &idxhash,
                                     tokenizer->word_hash,
                                     sentence0n + offset0n);

      if (sizedictionary != sizetoken) idxhash = -1;
    } else {
      SENNA_tokenize_alphanumeric(&sizealphanumeric, sentence0n + offset0n);
      SENNA_tokenize_dictionarymatch(&sizedictionary, &idxhash,
                                     tokenizer->word_hash,
                                     sentence0n + offset0n);

      sizetoken = (sizealphanumeric > sizedictionary ? sizealphanumeric
                                                     : sizedictionary);
      idxhash = (sizealphanumeric > sizedictionary ? -1 : idxhash);
    }

    if (sizetoken == 0) {
      if (sentence0n[offset0n] == '\0')
        break;
      else {
        fprintf(stderr, "WARNING: skipping a char (%c)\n",
                sentence0n[offset0n]);
        offset0n++;
        continue;
      }
    }

    /* check buffer sizes */
    /* note that we increment one at the time */
    incsize = 0;
    if (tokenizer->tokens->n + 1 > tokenizer->max_tokens) {
      tokenizer->max_tokens = tokenizer->tokens->n + 1;
      incsize = 1;
    }

    /* word strings */
    {
      int sizetokenraw = tokenizer->offset0n2raw[offset0n + sizetoken] -
                         tokenizer->offset0n2raw[offset0n];

      if (incsize) {
        tokenizer->tokens->words = SENNA_realloc(
            tokenizer->tokens->words, sizeof(char *), tokenizer->max_tokens);
        tokenizer->tokens->words[tokenizer->tokens->n] = NULL;

        tokenizer->words_sizes = SENNA_realloc(
            tokenizer->words_sizes, sizeof(int), tokenizer->max_tokens);
        tokenizer->words_sizes[tokenizer->tokens->n] = 0;
      }

      if (sizetokenraw >= tokenizer->words_sizes[tokenizer->tokens->n]) {
        tokenizer->words_sizes[tokenizer->tokens->n] = sizetokenraw + 1;
        tokenizer->tokens->words[tokenizer->tokens->n] = SENNA_realloc(
            tokenizer->tokens->words[tokenizer->tokens->n], sizeof(char),
            tokenizer->words_sizes[tokenizer->tokens->n]);
      }

      memcpy(tokenizer->tokens->words[tokenizer->tokens->n],
             sentence + tokenizer->offset0n2raw[offset0n], sizetokenraw);
      tokenizer->tokens->words[tokenizer->tokens->n][sizetokenraw] = '\0';
    }

    /* words */
    {
      if (incsize)
        tokenizer->tokens->word_idx = SENNA_realloc(
            tokenizer->tokens->word_idx, sizeof(int), tokenizer->max_tokens);
      tokenizer->tokens->word_idx[tokenizer->tokens->n] =
          (idxhash >= 0 ? idxhash : tokenizer->word_hash_unknown_idx);
    }

    /* word offsets */
    {
      if (incsize) {
        tokenizer->tokens->start_offset =
            SENNA_realloc(tokenizer->tokens->start_offset, sizeof(int),
                          tokenizer->max_tokens);
        tokenizer->tokens->end_offset = SENNA_realloc(
            tokenizer->tokens->end_offset, sizeof(int), tokenizer->max_tokens);
      }
      tokenizer->tokens->start_offset[tokenizer->tokens->n] =
          tokenizer->offset0n2raw[offset0n];
      tokenizer->tokens->end_offset[tokenizer->tokens->n] =
          tokenizer->offset0n2raw[offset0n + sizetoken];
    }

    /* caps */
    if (tokenizer->caps_hash) {
      int i;
      int allcaps, initcap, hascap;

      allcaps = !islower(sentence0n[offset0n]);
      initcap = isupper(sentence0n[offset0n]);
      hascap = initcap;

      for (i = 1; i < sizetoken; i++) {
        if (islower(sentence0n[offset0n + i]))
          allcaps = 0;
        else if (isupper(sentence0n[offset0n + i]))
          hascap = 1;
      }

      if (incsize)
        tokenizer->tokens->caps_idx = SENNA_realloc(
            tokenizer->tokens->caps_idx, sizeof(int), tokenizer->max_tokens);

      if (hascap && allcaps)
        tokenizer->tokens->caps_idx[tokenizer->tokens->n] =
            tokenizer->caps_hash_allcaps_idx;
      else if (initcap)
        tokenizer->tokens->caps_idx[tokenizer->tokens->n] =
            tokenizer->caps_hash_initcap_idx;
      else if (hascap)
        tokenizer->tokens->caps_idx[tokenizer->tokens->n] =
            tokenizer->caps_hash_hascap_idx;
      else
        tokenizer->tokens->caps_idx[tokenizer->tokens->n] =
            tokenizer->caps_hash_nocaps_idx;
    }

    /* suffixes */
    if (tokenizer->suff_hash) {
      static char suffix[3] = "\0\0\0";
      int idxhashsuffix;

      suffix[0] =
          (char)(sizetoken >= 2
                     ? tolower(sentence0n[offset0n + sizetoken - 2])
                     : (sizetoken >= 1
                            ? tolower(sentence0n[offset0n + sizetoken - 1])
                            : '\0'));
      suffix[1] =
          (char)(sizetoken >= 2 ? tolower(sentence0n[offset0n + sizetoken - 1])
                                : '\0');

      idxhashsuffix = SENNA_Hash_index(tokenizer->suff_hash, suffix);

      if (incsize)
        tokenizer->tokens->suff_idx = SENNA_realloc(
            tokenizer->tokens->suff_idx, sizeof(int), tokenizer->max_tokens);
      tokenizer->tokens->suff_idx[tokenizer->tokens->n] =
          (idxhashsuffix < 0 ? tokenizer->suff_hash_nosuffix_idx
                             : idxhashsuffix);
    }

    tokenizer->tokens->n++;
    offset0n = offset0n + sizetoken;
  }

  /* gazetteers */
  /* note: they need to know all the tokens, so we do it at the end */
  if (tokenizer->gazl_hash)
    tokenize_gazetteer(&tokenizer->tokens->gazl_idx, tokenizer,
                       tokenizer->gazl_hash);

  if (tokenizer->gazm_hash)
    tokenize_gazetteer(&tokenizer->tokens->gazm_idx, tokenizer,
                       tokenizer->gazm_hash);

  if (tokenizer->gazo_hash)
    tokenize_gazetteer(&tokenizer->tokens->gazo_idx, tokenizer,
                       tokenizer->gazo_hash);

  if (tokenizer->gazp_hash)
    tokenize_gazetteer(&tokenizer->tokens->gazp_idx, tokenizer,
                       tokenizer->gazp_hash);

  return tokenizer->tokens;
}

void SENNA_tokenize_untilspace(int *size_, const char *sentence) {
  int size = 0;
  while (1) {
    char c = sentence[size];
    if (c == '\0' || isspace(c)) break;
    size++;
  }
  *size_ = size;
}

void SENNA_tokenize_alphanumeric(int *size_, const char *sentence) {
  int size = 0;
  while (1) {
    char c = *sentence++;
    if (c == '\0' || (!isdigit(c) && !isalpha(c))) break;
    size++;
  }
  *size_ = size;
}

void SENNA_tokenize_dictionarymatch(int *size_, int *idxhash_, SENNA_Hash *hash,
                                    const char *sentence) {
  static char word[MAX_WORD_SIZE];
  int size = 0;
  int idxhash = -1;
  char c;

  /* match until space */
  while (size < MAX_WORD_SIZE - 1) {
    c = sentence[size];
    if (c == '\0' || isspace(c)) break;
    word[size++] = (char)tolower(c);
  }

  /* take the largest word into the dictionary */
  for (; size > 0; size--) {
    word[size] = '\0';
    idxhash = SENNA_Hash_index(hash, word);

    if (idxhash >= 0) break;
  }

  *size_ = size;
  *idxhash_ = idxhash;
}

void SENNA_tokenize_number(int *size_, const char *sentence) {
  int state = 0;
  int size = 0;
  int idx = 0;
  int finished = 0;

  while (!finished) {
    char c = sentence[idx++];

    if (c == '\0') break;

    switch (state) {
      case 0:
        if (c == '+' || c == '-')
          state = 1;
        else if (c == '.' || c == ',')
          state = 2;
        else if (isdigit(c))
          state = 4;
        else
          finished = 1;
        break;

      case 1:
        if (c == '.' || c == ',')
          state = 2;
        else if (isdigit(c))
          state = 4;
        else
          finished = 1;
        break;

      case 2:
        if (isdigit(c))
          state = 4;
        else
          finished = 1;
        break;

      case 3:
        if (isdigit(c))
          state = 4;
        else
          finished = 1;
        break;

      case 4:
        size = idx - 1;
        if (c == '.' || c == ',')
          state = 3;
        else if (isdigit(c))
          state = 4;
        else
          finished = 1;
        break;
    }
  }

  *size_ = size;
}

static int checkhash(SENNA_Hash *hash, const char *key) {
  int idx;

  if (!hash) return -1;

  idx = SENNA_Hash_index(hash, key);
  if (idx < 0) SENNA_error("could not find key %s", key);

  return idx;
}

SENNA_Tokenizer *SENNA_Tokenizer_new(
    SENNA_Hash *word_hash, SENNA_Hash *caps_hash, SENNA_Hash *suff_hash,
    SENNA_Hash *gazt_hash, SENNA_Hash *gazl_hash, SENNA_Hash *gazm_hash,
    SENNA_Hash *gazo_hash, SENNA_Hash *gazp_hash, int is_tokenized) {
  SENNA_Tokenizer *tokenizer = SENNA_malloc(sizeof(SENNA_Tokenizer), 1);
  memset(tokenizer, 0, sizeof(SENNA_Tokenizer));

  if (!word_hash) SENNA_error("Tokenizer *needs* a hash for words");

  tokenizer->is_tokenized = is_tokenized;

  tokenizer->word_hash = word_hash;
  tokenizer->caps_hash = caps_hash;
  tokenizer->suff_hash = suff_hash;
  tokenizer->gazt_hash = gazt_hash;
  tokenizer->gazl_hash = gazl_hash;
  tokenizer->gazm_hash = gazm_hash;
  tokenizer->gazo_hash = gazo_hash;
  tokenizer->gazp_hash = gazp_hash;

  tokenizer->word_hash_unknown_idx = checkhash(word_hash, "UNKNOWN");
  tokenizer->suff_hash_nosuffix_idx = checkhash(suff_hash, "NOSUFFIX");
  tokenizer->caps_hash_allcaps_idx = checkhash(caps_hash, "allcaps");
  tokenizer->caps_hash_hascap_idx = checkhash(caps_hash, "hascap");
  tokenizer->caps_hash_initcap_idx = checkhash(caps_hash, "initcap");
  tokenizer->caps_hash_nocaps_idx = checkhash(caps_hash, "nocaps");
  tokenizer->gazt_hash_no_entity_idx = checkhash(gazt_hash, "NO");
  tokenizer->gazt_hash_is_entity_idx = checkhash(gazt_hash, "YES");

  tokenizer->tokens = SENNA_malloc(sizeof(SENNA_Tokens), 1);
  memset(tokenizer->tokens, 0, sizeof(SENNA_Tokens));

  return tokenizer;
}

void SENNA_Tokenizer_free(SENNA_Tokenizer *tokenizer) {
  int i;

  for (i = 0; i < tokenizer->max_tokens; i++)
    SENNA_free(tokenizer->tokens->words[i]);
  SENNA_free(tokenizer->tokens->words);
  SENNA_free(tokenizer->tokens->start_offset);
  SENNA_free(tokenizer->tokens->end_offset);
  SENNA_free(tokenizer->tokens->word_idx);
  SENNA_free(tokenizer->tokens->caps_idx);
  SENNA_free(tokenizer->tokens->suff_idx);
  SENNA_free(tokenizer->tokens->gazl_idx);
  SENNA_free(tokenizer->tokens->gazm_idx);
  SENNA_free(tokenizer->tokens->gazo_idx);
  SENNA_free(tokenizer->tokens->gazp_idx);
  SENNA_free(tokenizer->tokens);

  SENNA_free(tokenizer->words_sizes);
  SENNA_free(tokenizer->sentence0n);
  SENNA_free(tokenizer->offset0n2raw);
  SENNA_free(tokenizer->entity);

  SENNA_free(tokenizer);
}

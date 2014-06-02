//
//  CRF++ -- Yet Another CRF toolkit
//
//  $Id: libcrfpp.cpp 1587 2007-02-12 09:00:36Z taku $;
//
//  Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#endif

#include <string>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "crfpp.h"
#include "scoped_ptr.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
namespace CRFPP {
std::wstring Utf8ToWide(const std::string &input) {
  int output_length = ::MultiByteToWideChar(CP_UTF8, 0,
                                            input.c_str(), -1, NULL, 0);
  output_length = output_length <= 0 ? 0 : output_length - 1;
  if (output_length == 0) {
    return L"";
  }
  scoped_array<wchar_t> input_wide(new wchar_t[output_length + 1]);
  const int result = ::MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1,
                                           input_wide.get(), output_length + 1);
  std::wstring output;
  if (result > 0) {
    output.assign(input_wide.get());
  }
  return output;
}

std::string WideToUtf8(const std::wstring &input) {
  const int output_length = ::WideCharToMultiByte(CP_UTF8, 0,
                                                  input.c_str(), -1, NULL, 0,
                                                  NULL, NULL);
  if (output_length == 0) {
    return "";
  }

  scoped_array<char> input_encoded(new char[output_length + 1]);
  const int result = ::WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1,
                                           input_encoded.get(),
                                           output_length + 1, NULL, NULL);
  std::string output;
  if (result > 0) {
    output.assign(input_encoded.get());
  }
  return output;
}
}  // CRFPP
#endif

crfpp_model_t* crfpp_model_new(int argc,  char **argv) {
  CRFPP::Model *model = CRFPP::createModel(argc, argv);
  if (!model) {
    return 0;
  }
  return reinterpret_cast<crfpp_model_t *>(model);
}

crfpp_model_t* crfpp_model_new2(const char *arg) {
  CRFPP::Model *model = CRFPP::createModel(arg);
  if (!model) {
    return 0;
  }
  return reinterpret_cast<crfpp_model_t *>(model);
}

crfpp_model_t* crfpp_model_from_array_new(int argc,  char **argv,
                                          const char *buf,
                                          size_t size) {
  CRFPP::Model *model = CRFPP::createModelFromArray(argc, argv,
                                                    buf, size);
  if (!model) {
    return 0;
  }
  return reinterpret_cast<crfpp_model_t *>(model);
}

crfpp_model_t* crfpp_model_from_array_new2(const char *arg,
                                           const char *buf,
                                           size_t size) {
  CRFPP::Model *model = CRFPP::createModelFromArray(arg,
                                                    buf, size);
  if (!model) {
    return 0;
  }
  return reinterpret_cast<crfpp_model_t *>(model);
}

const char *   crfpp_model_get_template(crfpp_model_t *c) {
  return reinterpret_cast<const char *>(
      reinterpret_cast<CRFPP::Model *>(c)->getTemplate());
}

void crfpp_model_destroy(crfpp_model_t *c) {
  CRFPP::Model *model = reinterpret_cast<CRFPP::Model *>(c);
  delete model;
}

const char* crfpp_model_strerror(crfpp_model_t *c) {
  if (!c) {
    return CRFPP::getLastError();
  }
  return reinterpret_cast<CRFPP::Model *>(c)->what();
}

crfpp_t* crfpp_model_new_tagger(crfpp_model_t *c) {
  return reinterpret_cast<crfpp_t *>(
      reinterpret_cast<CRFPP::Model *>(c)->createTagger());
}

crfpp_t* crfpp_new(int argc, char **argv) {
  CRFPP::Tagger *tagger = CRFPP::createTagger(argc, argv);
  if (!tagger) {
    return 0;
  }
  return reinterpret_cast<crfpp_t *>(tagger);
}

crfpp_t* crfpp_new2(char *arg) {
  CRFPP::Tagger *tagger = CRFPP::createTagger(arg);
  if (!tagger) {
    return 0;
  }
  return reinterpret_cast<crfpp_t *>(tagger);
}

const char* crfpp_strerror(crfpp_t *c) {
  if (!c) {
    return CRFPP::getLastError();
  }
  return reinterpret_cast<CRFPP::Tagger *>(c)->what();
}

void crfpp_destroy(crfpp_t *c) {
  CRFPP::Tagger *tagger = reinterpret_cast<CRFPP::Tagger *>(c);
  delete tagger;
}

int      crfpp_set_model(crfpp_t *c, crfpp_model_t *model) {
  return static_cast<int>(reinterpret_cast<CRFPP::Tagger *>(c)->set_model(
                              reinterpret_cast<const CRFPP::Model &>(*model)));
}

int      crfpp_add2(crfpp_t* c, size_t s, const char **line) {
  return static_cast<int>(reinterpret_cast<CRFPP::Tagger *>(c)->add(s, line));
}

int      crfpp_add(crfpp_t* c, const char*s) {
  return static_cast<int>(reinterpret_cast<CRFPP::Tagger *>(c)->add(s));
}

size_t   crfpp_size(crfpp_t* c) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->size();
}

size_t   crfpp_xsize(crfpp_t* c) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->xsize();
}

size_t   crfpp_result(crfpp_t* c, size_t i) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->result(i);
}

size_t   crfpp_answer(crfpp_t* c, size_t i) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->answer(i);
}

size_t   crfpp_y(crfpp_t* c, size_t i) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->y(i);
}

size_t   crfpp_ysize(crfpp_t* c) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->ysize();
}

double   crfpp_prob(crfpp_t* c, size_t i, size_t j) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->prob(i, j);
}

double   crfpp_prob2(crfpp_t* c, size_t i) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->prob(i);
}

double   crfpp_prob3(crfpp_t* c) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->prob();
}

void     crfpp_set_penalty(crfpp_t *c, size_t i, size_t j, double penalty) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->set_penalty(i, j, penalty);
}

double   crfpp_penalty(crfpp_t *c, size_t i, size_t j) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->penalty(i, j);
}

double   crfpp_alpha(crfpp_t* c, size_t i, size_t j) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->alpha(i, j);
}

double   crfpp_beta(crfpp_t* c, size_t i, size_t j) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->beta(i, j);
}

double   crfpp_best_cost(crfpp_t* c, size_t i, size_t j) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->best_cost(i, j);
}

double   crfpp_emisstion_cost(crfpp_t* c, size_t i, size_t j) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->emission_cost(i, j);
}

const int* crfpp_emisstion_vector(crfpp_t* c, size_t i, size_t j) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->emission_vector(i, j);
}

double   crfpp_next_transition_cost(crfpp_t* c, size_t i, size_t j, size_t k) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->next_transition_cost(i, j, k);
}

double   crfpp_prev_transition_cost(crfpp_t* c, size_t i, size_t j, size_t k) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->next_transition_cost(i, j, k);
}

const  int* crfpp_next_transition_vector(crfpp_t* c, size_t i,
                                         size_t j, size_t k) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->next_transition_vector(i, j, k);
}

const int* crfpp_prev_transition_vector(crfpp_t* c, size_t i,
                                        size_t j, size_t k) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->next_transition_vector(i, j, k);
}

size_t crfpp_dsize(crfpp_t* c) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->dsize();
}

const float* crfpp_weight_vector(crfpp_t* c) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->weight_vector();
}

double   crfpp_Z(crfpp_t* c) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->Z();
}

int      crfpp_parse(crfpp_t* c) {
  return static_cast<int>(reinterpret_cast<CRFPP::Tagger *>(c)->parse());
}

int      crfpp_empty(crfpp_t* c) {
  return static_cast<int>(reinterpret_cast<CRFPP::Tagger *>(c)->empty());
}

int      crfpp_clear(crfpp_t* c) {
  return static_cast<int>(reinterpret_cast<CRFPP::Tagger *>(c)->clear());
}

int      crfpp_next(crfpp_t* c) {
  return static_cast<int>(reinterpret_cast<CRFPP::Tagger *>(c)->next());
}

const char*   crfpp_yname(crfpp_t* c, size_t i) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->yname(i);
}

const char*   crfpp_y2(crfpp_t* c, size_t i) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->y2(i);
}

const char*   crfpp_x(crfpp_t* c, size_t i, size_t j) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->x(i, j);
}

const char**  crfpp_x2(crfpp_t* c, size_t i) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->x(i);
}

const char*  crfpp_parse_tostr(crfpp_t* c, const char* str) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->parse(str);
}

const char*  crfpp_parse_tostr2(crfpp_t* c, const char* str, size_t len) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->parse(str, len);
}

const char*  crfpp_parse_tostr3(crfpp_t* c, const char* str,
                                size_t len, char *ostr, size_t len2) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->parse(str, len, ostr, len2);
}

const char*  crfpp_tostr(crfpp_t* c) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->toString();
}

const char*  crfpp_tostr2(crfpp_t* c, char *ostr, size_t len) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->toString(ostr, len);
}

void crfpp_set_vlevel(crfpp_t *c, unsigned int vlevel) {
  reinterpret_cast<CRFPP::Tagger *>(c)->set_vlevel(vlevel);
}

unsigned int crfpp_vlevel(crfpp_t *c) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->vlevel();
}

void crfpp_set_cost_factor(crfpp_t *c, float cost_factor) {
  reinterpret_cast<CRFPP::Tagger *>(c)->set_cost_factor(cost_factor);
}

float crfpp_cost_factor(crfpp_t *c) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->cost_factor();
}

void crfpp_set_nbest(crfpp_t *c, size_t nbest) {
  reinterpret_cast<CRFPP::Tagger *>(c)->set_nbest(nbest);
}

size_t crfpp_nbest(crfpp_t *c) {
  return reinterpret_cast<CRFPP::Tagger *>(c)->nbest();
}

%module CRFPP
%include exception.i
%{
#include "crfpp.h"
%}

%newobject surface;

%exception {
  try { $action }
  catch (char *e) { SWIG_exception (SWIG_RuntimeError, e); }
  catch (const char *e) { SWIG_exception (SWIG_RuntimeError, (char*)e); }
}

%feature("notabstract") CRFPP::Model;
%feature("notabstract") CRFPP::Tagger;
%ignore CRFPP::createModel;
%ignore CRFPP::createModelFromArray;
%ignore CRFPP::createTagger;
%ignore CRFPP::getTaggerError;
%ignore CRFPP::getLastError;

%extend CRFPP::Model { Model(const char *arg); }
%extend CRFPP::Tagger { Tagger(const char *arg); }

%{

void delete_CRFPP_Model (CRFPP::Model *t) {
  delete t;
  t = 0;
}

CRFPP::Model* new_CRFPP_Model(const char *arg) {
  CRFPP::Model *tagger = CRFPP::createModel(arg);
  if (!tagger) throw CRFPP::getLastError();
  return tagger;
}

void delete_CRFPP_Tagger (CRFPP::Tagger *t) {
  delete t;
  t = 0;
}

CRFPP::Tagger* new_CRFPP_Tagger (const char *arg) {
  CRFPP::Tagger *tagger = CRFPP::createTagger(arg);
  if (!tagger) throw CRFPP::getLastError();
  return tagger;
}

%}

%include ../crfpp.h
%include version.h

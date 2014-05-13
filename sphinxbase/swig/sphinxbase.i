/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2013 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced
 * Research Projects Agency and the National Science Foundation of the
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */


%define DOCSTRING
"This documentation was automatically generated using original comments in
Doxygen format. As some C types and data structures cannot be directly mapped
into Python types, some non-trivial type conversion could have place.
Basically a type is replaced with another one that has the closest match, and
sometimes one argument of generated function comprises several arguments of the
original function (usually two). Apparently Doxygen comments do not mention
this fact, so here is a list of all known conversions so far:

  FILE * -> file
  const int16 *SDATA, size_t NSAMP -> str

Also functions having error code as the return value and returning effective
value in one of its arguments are transformed so that the effective value is
returned in a regular fashion and run-time exception is being thrown in case of
negative error code."
%enddef

#if SWIGJAVA
%module SphinxBase
#else
%module(docstring=DOCSTRING) sphinxbase
#endif

%feature("autodoc", "1");

%include typemaps.i

%begin %{
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/err.h>
#include <sphinxbase/fe.h>
#include <sphinxbase/feat.h>
#include <sphinxbase/jsgf.h>
#include <sphinxbase/ngram_model.h>

typedef int bool;
#define false 0
#define true 1

typedef cmd_ln_t Config;
typedef jsgf_t Jsgf;
typedef jsgf_rule_t JsgfRule;
typedef feat_t Feature;
typedef fe_t FrontEnd;
typedef fsg_model_t FsgModel;
typedef logmath_t LogMath;
typedef ngram_model_t NGramModel;
typedef ngram_model_t NGramModelSet;
%}

%nodefaultctor Config;

typedef struct {} Config;
typedef struct {} FrontEnd;
typedef struct {} Feature;
typedef struct {} FsgModel;
typedef struct {} JsgfRule;
typedef struct {} NGramModel;

iterable(NGramModelSet, ngram_model_set, NGramModel)
iterable(Jsgf, jsgf_rule, JsgfRule)

#ifdef HAS_DOC
%include pydoc.i
#endif
%include cmd_ln.i
%include fe.i
%include feat.i
%include fsg_model.i
%include jsgf.i
%include ngram_model.i

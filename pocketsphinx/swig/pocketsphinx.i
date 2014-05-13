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
%module PocketSphinx
#else
%module(docstring=DOCSTRING) pocketsphinx
#endif

%feature("autodoc", "1");

%include typemaps.i

%import sphinxbase.i

// TODO: use %newobject in a couple with ckd_malloc/ckd_free
// TODO: split SegmentIterator off Segment
// TODO: split NBestIterator off NBest

// TODO: probably these definitions should be imported from sphinxbase
%{
typedef cmd_ln_t Config;
typedef feat_t Feature;
typedef fe_t FrontEnd;
typedef fsg_model_t FsgModel;
typedef logmath_t LogMath;
typedef ngram_model_t NGramModel;
typedef ngram_model_t NGramModelSet;
%}

%begin %{
#include <pocketsphinx.h>

typedef int bool;
#define false 0
#define true 1

typedef ps_decoder_t Decoder;
typedef ps_lattice_t Lattice;
%}

%inline %{

// TODO: make private with %immutable
typedef struct {
    char *hypstr;
    char *uttid;
    int best_score;
} Hypothesis;

typedef struct {
    ps_seg_t *ptr;
    char *word;
    int32 ascr;
    int32 lscr;
    int32 lback;
    int start_frame;
    int end_frame;
} Segment;

typedef struct {
    ps_nbest_t *ptr;
} NBest;
%}

typedef struct {} Decoder;
typedef struct {} Lattice;

#ifdef HAS_DOC
%include pydoc.i
#endif
%include ps_decoder.i
%include ps_lattice.i

%extend Hypothesis {
    Hypothesis(char const *hypstr, char const *uttid, int best_score) {
        Hypothesis *h = ckd_malloc(sizeof *h);
        if (hypstr)
            h->hypstr = ckd_salloc(hypstr);
        if (uttid)
            h->uttid = ckd_salloc(uttid);
        h->best_score = best_score;
        return h;  
  }

    ~Hypothesis() {
        ckd_free($self->hypstr);
        ckd_free($self->uttid);
        ckd_free($self);
    }
}

#ifdef SWIGPYTHON
%exception NBest::next() {
    $action
    if (!arg1->ptr) {
        PyErr_SetString(PyExc_StopIteration, "");
        SWIG_fail;
    }
}
#endif

%extend NBest {
    NBest(ps_nbest_t *ptr) {
        if (!ptr)
            return NULL;

        NBest *nbest = ckd_malloc(sizeof *nbest);
        nbest->ptr = ptr;

        return nbest;
    }

    ~NBest() {
          ps_nbest_free($self->ptr);
          ckd_free($self);
    }
  
#ifdef SWIGPYTHON

%pythoncode %{
    def __iter__(self):
        return self
%}

    NBest * next() {
        $self->ptr = ps_nbest_next($self->ptr);
        return $self;
    }
#endif
  
    %newobject hyp;
    Hypothesis * hyp() {
        int32 score;
        const char *hyp = ps_nbest_hyp($self->ptr, &score);
        // TODO: refactor; what is this empty argument?
        return new_Hypothesis(hyp, "", score);
    }

    %newobject seg;
    Segment * seg() {
        int32 score;
        // TODO: refactor; use 'score' value
        return new_Segment(ps_nbest_seg($self->ptr, &score));
    }
}

#ifdef SWIGPYTHON
%exception Segment::next() {
    $action
    if (!arg1->ptr) {
        PyErr_SetString(PyExc_StopIteration, "");
        SWIG_fail;
    }
}
#endif

%extend Segment {
    Segment(ps_seg_t *ptr) {
        if (!ptr)
            return NULL;

        Segment *seg = ckd_malloc(sizeof *seg);
        seg->ptr = ptr;
        seg->word = 0;

        return seg;
    }

    ~Segment() {
        if ($self->ptr)
    	    ps_seg_free($self->ptr);
        ckd_free($self->word);
        ckd_free($self);
    }

#ifdef SWIGPYTHON

%pythoncode %{
    def __iter__(self):
        return self
%}

    Segment* next() {
        if (($self->ptr = ps_seg_next($self->ptr))) {
            ckd_free($self->word);
            $self->word = ckd_salloc(ps_seg_word($self->ptr));
            ps_seg_prob($self->ptr, &$self->ascr, &$self->lscr, &$self->lback);
            ps_seg_frames($self->ptr, &$self->start_frame, &$self->end_frame);
        }
        return $self;
    }
    
#endif

}

/* TODO: find out if it's possible to move these to Decoder */
%constant char *SEARCH_FSG = PS_SEARCH_FSG;
%constant char *SEARCH_NGRAM = PS_SEARCH_NGRAM;

/* ====================================================================
 * Copyright (c) 2006 Carnegie Mellon University.  All rights 
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

#ifndef FE_WARP_H
#define FE_WARP_H

#include "fe_internal.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

#define FE_WARP_ID_INVERSE_LINEAR	0
#define FE_WARP_ID_AFFINE	        1
#define FE_WARP_ID_PIECEWISE_LINEAR	2
#define FE_WARP_ID_EIDE_GISH		3
#define FE_WARP_ID_MAX		        2
#define FE_WARP_ID_NONE	       0xffffffff

typedef struct {
    void (*set_parameters)(char const *param_str, float sampling_rate);
    const char * (*doc)(void);
    uint32 (*id)(void);
    uint32 (*n_param)(void);
    float (*warped_to_unwarped)(float nonlinear);
    float (*unwarped_to_warped)(float linear);
    void (*print)(const char *label);
} fe_warp_conf_t;

int fe_warp_set(melfb_t *mel, const char *id_name);

uint32 fe_warp_id(melfb_t *mel);

const char * fe_warp_doc(melfb_t *mel);

void fe_warp_set_parameters(melfb_t *mel, char const *param_str, float sampling_rate);

uint32 fe_warp_n_param(melfb_t *mel);

float fe_warp_warped_to_unwarped(melfb_t *mel, float nonlinear);

float fe_warp_unwarped_to_warped(melfb_t *mel, float linear);

void fe_warp_print(melfb_t *mel, const char *label);

#define FE_WARP_NO_SIZE	0xffffffff

#ifdef __cplusplus
}
#endif


#endif /* FE_WARP_H */ 

/*
 * Log record.  Maintained by RCS.
 *
 * $Log: fe_warp.h,v $
 * Revision 1.2  2006/02/17 00:31:34  egouvea
 * Removed switch -melwarp. Changed the default for window length to
 * 0.025625 from 0.256 (so that a window at 16kHz sampling rate has
 * exactly 410 samples). Cleaned up include's. Replaced some E_FATAL()
 * with E_WARN() and return.
 *
 * Revision 1.1  2006/02/16 00:18:26  egouvea
 * Implemented flexible warping function. The user can specify at run
 * time which of several shapes they want to use. Currently implemented
 * are an affine function (y = ax + b), an inverse linear (y = a/x) and a
 * piecewise linear (y = ax, up to a frequency F, and then it "breaks" so
 * Nyquist frequency matches in both scales.
 *
 * Added two switches, -warp_type and -warp_params. The first specifies
 * the type, which valid values:
 *
 * -inverse or inverse_linear
 * -linear or affine
 * -piecewise or piecewise_linear
 *
 * The inverse_linear is the same as implemented by EHT. The -mel_warp
 * switch was kept for compatibility (maybe remove it in the
 * future?). The code is compatible with EHT's changes: cepstra created
 * from code after his changes should be the same as now. Scripts that
 * worked with his changes should work now without changes. Tested a few
 * cases, same results.
 *
 */

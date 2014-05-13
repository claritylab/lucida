/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2007 Carnegie Mellon University.  All rights
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
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifndef __GST_POCKETSPHINX_H__
#define __GST_POCKETSPHINX_H__

#include <gst/gst.h>
#include <pocketsphinx.h>

G_BEGIN_DECLS

#define GST_TYPE_POCKETSPHINX                   \
    (gst_pocketsphinx_get_type())
#define GST_POCKETSPHINX(obj)                                           \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_POCKETSPHINX,GstPocketSphinx))
#define GST_POCKETSPHINX_CLASS(klass)                                   \
    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_POCKETSPHINX,GstPocketSphinxClass))
#define GST_IS_POCKETSPHINX(obj)                                \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_POCKETSPHINX))
#define GST_IS_POCKETSPHINX_CLASS(klass)                        \
    (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_POCKETSPHINX))

typedef struct _GstPocketSphinx      GstPocketSphinx;
typedef struct _GstPocketSphinxClass GstPocketSphinxClass;

struct _GstPocketSphinx
{
    GstElement element;

    GstPad *sinkpad, *srcpad;

    ps_decoder_t *ps;
    cmd_ln_t *config;
    gchar *latdir;                 /**< Output directory for word lattices. */
    int n_best_size;               /**< Size of the returned B-best list */

    GHashTable *arghash;
    gboolean listening;

    GstClockTime last_result_time; /**< Timestamp of last partial result. */
    char *last_result;             /**< String of last partial result. */
};

struct _GstPocketSphinxClass 
{
    GstElementClass parent_class;

    void (*partial_result)  (GstElement *element, const gchar *hyp_str);
    void (*result)          (GstElement *element, const gchar *hyp_str);
};

GType gst_pocketsphinx_get_type(void);

/*
 * Boxing of lattices.
 */
#define PS_LATTICE_TYPE (ps_lattice_get_type())
GType ps_lattice_get_type(void);

/*
 * Boxing of decoder.
 */
#define PS_DECODER_TYPE (ps_decoder_get_type())
GType ps_decoder_get_type(void);

G_END_DECLS

#endif /* __GST_POCKETSPHINX_H__ */

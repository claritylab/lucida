/* ====================================================================
 * Copyright (c) 1999-2010 Carnegie Mellon University.  All rights
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

#ifndef __GST_VADER_H__
#define __GST_VADER_H__


#include <gst/gst.h>
#include <gst/gstevent.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#if 0
}
#endif

#define GST_TYPE_VADER				\
    (gst_vader_get_type())
#define GST_VADER(obj)                                          \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VADER,GstVader))
#define GST_VADER_CLASS(klass)						\
    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VADER,GstVaderClass))
#define GST_IS_VADER(obj)                               \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VADER))
#define GST_IS_VADER_CLASS(klass)                       \
    (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VADER))

/* Custom events inserted in the stream at start and stop of cuts. */
#define GST_EVENT_VADER_START						\
    GST_EVENT_MAKE_TYPE(146, GST_EVENT_TYPE_DOWNSTREAM | GST_EVENT_TYPE_SERIALIZED)
#define GST_EVENT_VADER_STOP						\
    GST_EVENT_MAKE_TYPE(147, GST_EVENT_TYPE_DOWNSTREAM | GST_EVENT_TYPE_SERIALIZED)

typedef struct _GstVader GstVader;
typedef struct _GstVaderClass GstVaderClass;

/* Maximum frame size over which VAD is calculated. */
#define VADER_FRAME 512
/* Number of frames over which to vote on speech/non-speech decision. */
#define VADER_WINDOW 5

struct _GstVader
{
    GstElement element;

    GstPad *sinkpad, *srcpad;

    GStaticRecMutex mtx;          /**< Lock used in setting parameters. */

    gboolean window[VADER_WINDOW];/**< Voting window of speech/silence decisions. */
    gboolean silent;		  /**< Current state of the filter. */
    gboolean silent_prev;	  /**< Previous state of the filter. */
    GList *pre_buffer;            /**< list of GstBuffers in pre-record buffer */
    guint64 silent_run_length;    /**< How much silence have we endured so far? */
    guint64 pre_run_length;       /**< How much pre-silence have we endured so far? */

    gint threshold_level;         /**< Silence threshold level (Q15, adaptive). */
    gint prior_sample;		  /**< Prior sample for pre-emphasis filter. */
    guint64 threshold_length;     /**< Minimum silence for cutting, in nanoseconds. */
    guint64 pre_length;           /**< Pre-buffer to add on silence->speech transition. */

    gboolean auto_threshold;      /**< Set threshold automatically. */
    gint silence_mean;            /**< Mean RMS power of silence frames. */
    gint silence_stddev;          /**< Variance in RMS power of silence frames. */
    gint silence_frames;          /**< Number of frames used in estimating mean/variance */

    gchar *dumpdir;               /**< Directory to dump audio to (for debugging). */
    FILE *dumpfile;		  /**< Current audio dump file. */
    gint dumpidx;                 /**< Dump file index. */
};

struct _GstVaderClass
{
    GstElementClass parent_class;
    void (*vader_start) (GstVader* filter);
    void (*vader_stop) (GstVader* filter);
};

GType gst_vader_get_type (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GST_VADER_H__ */

/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
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
/*
 * cont_ad.h -- Continuous A/D listening and silence filtering module.
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * 13-Jul-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added spf and adbufsize to cont_ad_t in order to support variable
 * 		frame sizes depending on audio sampling rate.
 * 
 * 30-Jun-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added FILE* argument to cont_ad_powhist_dump().
 * 
 * 16-Jan-98	Paul Placeway (pwp@cs.cmu.edu) at Carnegie Mellon University
 * 		Changed to use dB instead of the weird power measure.
 * 		Added most system parameters to cont_ad_t instead of hardwiring
 * 		them in cont_ad.c.
 * 		Added cont_ad_set_params() and cont_ad_get_params().
 * 
 * 28-Jul-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added cont_ad_t.siglvl.
 * 
 * 27-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added the option for cont_ad_read to return -1 on EOF.
 * 
 * 21-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added cont_ad_set_thresh().
 * 
 * 20-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Separated thresholds for speech and silence.
 * 
 * 17-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created, based loosely on Steve Reed's original implementation.
 */


#ifndef _CONT_AD_H_
#define _CONT_AD_H_

/* Win32/WinCE DLL gunk */
#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>
#include <sphinxbase/ad.h>

/**
 * \file cont_ad.h
 * \brief Continuous A/D listening and silence filtering module.
 *
 * This module is intended to be interposed as a filter between any
 * raw A/D source and the application to remove silence regions.  Its
 * main purpose is to remove regions of silence from the raw input
 * speech.  It is initialized with a raw A/D source function (during
 * the cont_ad_init call).  The application is responsible for setting
 * up the A/D source, turning recording on and off as it desires.
 * Filtered A/D data can be read by the application using the
 * cont_ad_read function.
 * 
 * In other words, the application calls cont_ad_read instead of the
 * raw A/D source function (e.g., ad_read in libad) to obtain filtered
 * A/D data with silence regions removed.  This module itself does not
 * enforce any other structural changes to the application.
 * 
 * The cont_ad_read function also updates an "absolute" timestamp (see
 * cont_ad_t.read_ts) at the end of each invocation.  The timestamp
 * indicates the total number of samples of A/D data read until this
 * point, including data discarded as silence frames.  The application
 * is responsible for using this timestamp to make any policy
 * decisions regarding utterance boundaries or whatever.
 */


#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/* States of continuous listening module */
#define CONT_AD_STATE_SIL	0
#define CONT_AD_STATE_SPEECH	1


/**
 * \struct spseg_t
 * \brief  (FOR INTERNAL USE ) Data structure for maintaining speech (non-silence) segments not yet consumed by the
 * application.  
 */
typedef struct spseg_s {
    int32 startfrm;	/**< Frame-id in adbuf (see below) of start of this segment */ 
    int32 nfrm;		/**< Number of frames in segment (may wrap around adbuf) */
    struct spseg_s *next;	/**< Next speech segment (with some intervening silence) */
} spseg_t;


/**
 * \struct cont_ad_t
 * \brief Continuous listening module or object
 * Continuous listening module or object.  An application can open and maintain several
 * such objects, if necessary.
 * FYI: Module always in one of two states: SILENCE or SPEECH.  Transitions between the
 * two detected by sliding a window spanning several frames and looking for some minimum
 * number of frames of the other type.
 */
typedef struct {
    /* Function to be called for obtaining A/D data (see prototype for ad_read in ad.h) */
    int32 (*adfunc)(ad_rec_t *ad, int16 *buf, int32 max);
    ad_rec_t *ad;	/**< A/D device argument for adfunc.  Also, ad->sps used to
			   determine frame size (spf, see below) */
    int32 rawmode;	/**< Pass all input data through, without filtering silence */
    
    int16 *adbuf;	/**< Circular buffer for maintaining A/D data read until consumed */

    /* **************************************************************************
     * state, read_ts, and siglvl are provided for READ-ONLY use by client
     * applications, and are updated by calls to cont_ad_read() (see below).  All
     * other variables should be left alone.
     */
    int32 state;	/**< State of data returned by most recent cont_ad_read call;
			   CONT_AD_STATE_SIL or CONT_AD_STATE_SPEECH. */
    int32 read_ts;	/**< Absolute timestamp (total no. of raw samples consumed
			   upto the most recent cont_ad_read call, starting from
			   the very beginning).  Note that this is a 32-bit
			   integer; applications should guard against overflow. */
    int32 seglen;	/**< Total no. of raw samples consumed in the segment
			   returned by the most recent cont_ad_read call.  Can be
			   used to detect silence segments that have stretched long
			   enough to terminate an utterance */
    int32 siglvl;	/**< Max signal level for the data consumed by the most recent
			   cont_ad_read call (dB range: 0-99).  Can be used to
			   update a V-U meter, for example. */
    /* ************************************************************************ */
    
    int32 sps;		/**< Samples/sec; moved from ad->sps to break dependence on
			   ad by N. Roy.*/

    int32 eof;		/**< Whether the source ad device has encountered EOF */
  
    int32 spf;		/**< Samples/frame; audio level is analyzed within frames */
    int32 adbufsize;	/**< Buffer size (Number of samples) */
    int32 prev_sample;	/**< For pre-emphasis filter */
    int32 headfrm;	/**< Frame number in adbuf with unconsumed A/D data */
    int32 n_frm;	/**< Number of complete frames of unconsumed A/D data in adbuf */
    int32 n_sample;	/**< Number of samples of unconsumed data in adbuf */
    int32 tot_frm;	/**< Total number of frames of A/D data read, including consumed ones */
    int32 noise_level;	/**< PWP: what we claim as the "current" noise level */
    
    int32 *pow_hist;	/**< Histogram of frame power, moving window, decayed */
    char *frm_pow;	/**< Frame power */

    int32 auto_thresh;  /**< Do automatic threshold adjustment or not */
    int32 delta_sil;	/**< Max silence power/frame ABOVE noise level */
    int32 delta_speech;	/**< Min speech power/frame ABOVE noise level */
    int32 min_noise;	/**< noise lower than this we ignore */
    int32 max_noise;	/**< noise higher than this signals an error */
    int32 winsize;	/**< how many frames to look at for speech det */
    int32 speech_onset;	/**< start speech on >= these many frames out of winsize, of >= delta_speech */
    int32 sil_onset;	/**< end speech on >= these many frames out of winsize, of <= delta_sil */
    int32 leader;	/**< pad beggining of speech with this many extra frms */
    int32 trailer;	/**< pad end of speech with this many extra frms */

    int32 thresh_speech;/**< Frame considered to be speech if power >= thresh_speech
			   (for transitioning from SILENCE to SPEECH state) */
    int32 thresh_sil;	/**< Frame considered to be silence if power <= thresh_sil
			   (for transitioning from SPEECH to SILENCE state) */
    int32 thresh_update;/**< Number of frames before next update to pow_hist/thresholds */
    float32 adapt_rate;	/**< Linear interpolation constant for rate at which noise level adapted
			   to each estimate;
			   range: 0-1; 0=> no adaptation, 1=> instant adaptation */
    
    int32 tail_state;	/**< State at the end of its internal buffer (internal use):
			   CONT_AD_STATE_SIL or CONT_AD_STATE_SPEECH.  Note: This is
			   different from cont_ad_t.state. */
    int32 win_startfrm;	/**< Where next analysis window begins */
    int32 win_validfrm;	/**< Number of frames currently available from win_startfrm for analysis */
    int32 n_other;	/**< If in SILENCE state, number of frames in analysis window considered to
			   be speech; otherwise number of frames considered to be silence */
    spseg_t *spseg_head;/**< First of unconsumed speech segments */
    spseg_t *spseg_tail;/**< Last of unconsumed speech segments */
    
    FILE *rawfp;	/**< If non-NULL, raw audio input data processed by cont_ad
			   is dumped to this file.  Controlled by user application
			   via cont_ad_set_rawfp().  NULL when cont_ad object is
			   initially created. */
    FILE *logfp;	/**< If non-NULL, write detailed logs of this object's
			   progress to the file.  Controlled by user application
			   via cont_ad_set_logfp().  NULL when cont_ad object is
			   initially created. */

    int32 n_calib_frame; /**< Number of frames of calibration data seen so far. */
} cont_ad_t;


/**
 * Initialize a continuous listening/silence filtering object.
 *
 * One time initialization of a continuous listening/silence filtering
 * object/module.  This can work in either "stream mode", where it
 * reads data from an audio device represented by
 * <code>ad_rec_t</code>, or in "block mode", where it filters out
 * silence regions from blocks of data passed into it.
 *
 * @param ad An audio device to read from, or NULL to operate in block mode.
 * @param adfunc The function used to read audio from <code>ad</code>,
 * or NULL to operate in block mode.  This is usually ad_read().
 * @return A pointer to a READ-ONLY structure used in other calls to
 * the object.  If any error occurs, the return value is NULL.
 */
SPHINXBASE_EXPORT
cont_ad_t *cont_ad_init (ad_rec_t *ad,	/**< In: The A/D source object to be filtered */
			 int32 (*adfunc)(ad_rec_t *ad, int16 *buf, int32 max)
			 /**< In: adfunc = source function to be invoked
					   to obtain raw A/D data.  See ad.h for the
					   required prototype definition. */
			 );

/**
 * Initializes a continuous listening object which simply passes data through (!)
 *
 * Like cont_ad_init, but put the module in raw mode; i.e., all data is passed
 * through, unfiltered.  (By special request.)
 */
SPHINXBASE_EXPORT
cont_ad_t *cont_ad_init_rawmode (ad_rec_t *ad,
				 int32 (*adfunc)(ad_rec_t *ad, int16 *buf, int32 max));


/**
 * Read raw audio data into the silence filter.
 *
 * The main read routine for reading speech/silence segmented audio data.  Audio
 * data is copied into the caller provided buffer, much like a file read routine.
 *
 * In "block mode", i.e. if NULL was passed as a read function to
 * <code>cont_ad_init</code>, the data in <code>buf</code> is taken as
 * input, and any non-silence data is written back to <code>buf</code>
 * on exit.  In this case, you must take care that <code>max</code>
 * does not overflow the internal buffer of the silence filter.  The
 * available number of samples can be obtained by calling
 * cont_ad_buffer_space().  Any excess data will be discarded.
 *
 * In normal mode, only speech segments are copied; silence segments are dropped.
 * In rawmode (cont_ad module initialized using cont_ad_init_rawmode()), all data
 * are passed through to the caller.  But, in either case, any single call to
 * cont_ad_read will never return data that crosses a speech/silence segment
 * boundary.
 * 
 * The following variables are updated for use by the caller (see cont_ad_t above):
 *   cont_ad_t.state,
 *   cont_ad_t.read_ts,
 *   cont_ad_t.seglen,
 *   cont_ad_t.siglvl.
 * 
 * Return value: Number of samples actually read, possibly 0; <0 if EOF on A/D source.
 */
SPHINXBASE_EXPORT
int32 cont_ad_read (cont_ad_t *r,	/**< In: Object pointer returned by cont_ad_init */
		    int16 *buf,		/**< In/Out: In block mode, contains input data.
                                           On return, buf contains A/D data returned
					   by this function, if any. */
		    int32 max		/**< In: Maximum number of samples to be filled into buf.
					   NOTE: max must be at least 256; otherwise
					   the functions returns -1. */
	);

/**
 * Get the maximum number of samples which can be passed into cont_ad_read().
 */
SPHINXBASE_EXPORT
int32 cont_ad_buffer_space(cont_ad_t *r);

/**
 * Calibrate the silence filter.
 *
 * Calibration to determine an initial silence threshold.  This function can be called
 * any number of times.  It should be called at least once immediately after cont_ad_init.
 * The silence threshold is also updated internally once in a while, so this function
 * only needs to be called in the middle if there is a definite change in the recording
 * environment.
 * The application is responsible for making sure that the raw audio source is turned on
 * before the calibration.
 * Return value: 0 if successful, <0 otherwise.
 */
SPHINXBASE_EXPORT
int32 cont_ad_calib (cont_ad_t *cont	/**< In: object pointer returned by cont_ad_init */
		     );

/**
 * Calibrate the silence filter without an audio device.
 *
 * If the application has not passed an audio device into the silence filter
 * at initialisation,  this routine can be used to calibrate the filter. The
 * buf (of length max samples) should contain audio data for calibration. This
 * data is assumed to be completely consumed. More than one call may be
 * necessary to fully calibrate. 
 * Return value: 0 if successful, <0 on failure, >0 if calibration not
 * complete.
 */
SPHINXBASE_EXPORT
int32 cont_ad_calib_loop (cont_ad_t *r, int16 *buf, int32 max); 

/**
 * Get the number of samples required to calibrate the silence filter.
 *
 * Since, as mentioned above, the calibration data is assumed to be
 * fully consumed, it may be desirable to "hold onto" this data in
 * case it contains useful speech.  This function returns the number
 * of samples required to calibrate the silence filter, which is
 * useful in allocating a buffer to store this data.
 *
 * @return Number of samples required for successful calibration.
 */
SPHINXBASE_EXPORT
int32 cont_ad_calib_size(cont_ad_t *r);

/**
 * Set silence and speech threshold parameters.
 *
 * The silence threshold is the max power
 * level, RELATIVE to the peak background noise level, in any silence frame.  Similarly,
 * the speech threshold is the min power level, RELATIVE to the peak background noise
 * level, in any speech frame.  In general, silence threshold <= speech threshold.
 * Increasing the thresholds (say, from the default value of 2 to 3 or 4) reduces the
 * sensitivity to background noise, but may also increase the chances of clipping actual
 * speech.
 * @return: 0 if successful, <0 otherwise.
 */
SPHINXBASE_EXPORT
int32 cont_ad_set_thresh (cont_ad_t *cont,	/**< In: Object ptr from cont_ad_init */
			  int32 sil,	/**< In: silence threshold (default 2) */
			  int32 sp	/**< In: speech threshold (default 2) */
			  );


/**
 * Set the changable parameters.
 *
 *   delta_sil, delta_speech, min_noise, and max_noise are in dB,
 *   winsize, speech_onset, sil_onset, leader and trailer are in frames of
 *   16 ms length (256 samples @ 16kHz sampling).
 */
SPHINXBASE_EXPORT
int32 cont_ad_set_params (cont_ad_t *r, int32 delta_sil, int32 delta_speech,
			  int32 min_noise, int32 max_noise,
			  int32 winsize, int32 speech_onset, int32 sil_onset,
			  int32 leader, int32 trailer,
			  float32 adapt_rate);

/**
 * PWP 1/14/98 -- get the changable params.
 *
 *   delta_sil, delta_speech, min_noise, and max_noise are in dB,
 *   winsize, speech_onset, sil_onset, leader and trailer are in frames of
 *   16 ms length (256 samples @ 16kHz sampling).
 */
SPHINXBASE_EXPORT
int32 cont_ad_get_params (cont_ad_t *r, int32 *delta_sil, int32 *delta_speech,
			  int32 *min_noise, int32 *max_noise,
			  int32 *winsize, int32 *speech_onset, int32 *sil_onset,
			  int32 *leader, int32 *trailer,
			  float32 *adapt_rate);

/**
 * Reset, discarding any accumulated speech segments.
 * @return 0 if successful, <0 otherwise.
 */
SPHINXBASE_EXPORT
int32 cont_ad_reset (cont_ad_t *cont);	/* In: Object pointer from cont_ad_init */


/**
 * Close the continuous listening object.
 */
SPHINXBASE_EXPORT
int32 cont_ad_close (cont_ad_t *cont);	/* In: Object pointer from cont_ad_init */


/**
 * Dump the power histogram.  For debugging...
 */
SPHINXBASE_EXPORT
void cont_ad_powhist_dump (FILE *fp, cont_ad_t *cont);


/**
 * Detach the given continuous listening module from the associated audio device.
 * @return 0 if successful, -1 otherwise.
 */
SPHINXBASE_EXPORT
int32 cont_ad_detach (cont_ad_t *c);


/**
 * Attach the continuous listening module to the given audio device/function.
 * (Like cont_ad_init, but without the calibration.)
 * @return 0 if successful, -1 otherwise.
 */
SPHINXBASE_EXPORT
int32 cont_ad_attach (cont_ad_t *c, ad_rec_t *a, int32 (*func)(ad_rec_t *, int16 *, int32));


/**
 * Set a file for dumping raw audio input.
 *
 * The application can ask cont_ad to dump the raw audio input that cont_ad
 * processes to a file.  Use this function to give the FILE* to the cont_ad
 * object.  If invoked with fp == NULL, dumping is turned off.  The application
 * is responsible for opening and closing the file.  If fp is non-NULL, cont_ad
 * assumes the file pointer is valid and opened for writing.
 *
 * @return 0 if successful, -1 otherwise.
 */
SPHINXBASE_EXPORT
int32 cont_ad_set_rawfp (cont_ad_t *c,	/* The cont_ad object being addressed */
			 FILE *fp);	/* File to which raw audio data is to
					   be dumped; NULL to stop dumping. */

/**
 * Set the file to which cont_ad logs its progress.
 *
 * Mainly for debugging.  If <code>fp</code> is NULL, logging is turned off.
 *
 * @return 0 if successful, -1 otherwise.
 */
SPHINXBASE_EXPORT
int32 cont_ad_set_logfp (cont_ad_t *c,	/* The cont_ad object being addressed */
			 FILE *fp);	/* File to which logs are written;
					   NULL to stop logging. */

/**
 * Set the silence and speech thresholds.
 *
 * For this to remain permanently in effect, the auto_thresh field of
 * the continuous listening module should be set to FALSE or 0.
 * Otherwise the thresholds may be modified by the noise- level
 * adaptation.
 */
SPHINXBASE_EXPORT
int32 cont_set_thresh(cont_ad_t *r, int32 silence, int32 speech);

#ifdef __cplusplus
}
#endif


#endif

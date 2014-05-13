/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2011 Glenn Pierce.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GLENN PIERCE ``AS IS'' AND 
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
/* Sphinx II libad (Linux)
 * ^^^^^^^^^^^^^^^^^^^^^^^
 *
 *
 * Glenn Pierce (glennpierce@gmail.com)
 * ********************************************************************
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <config.h>
#include <limits.h>

#include "prim_type.h"
#include "ad.h"

#include <jack/jack.h>

#define DEFAULT_DEVICE "system:capture_1"
#define BUFFER_SIZE 352800

/* #define MIC_SPEAKER_PASSTHROUGH_DEBUG */

const size_t sample_size = sizeof(jack_default_audio_sample_t);
const size_t int16_range_over_two = (-SHRT_MIN + SHRT_MAX) / 2.0;

int
process (jack_nframes_t nframes, void *arg)
{
    ad_rec_t *handle = (ad_rec_t *) arg;

    size_t buffer_size = jack_ringbuffer_write_space (handle->rbuffer); 

    jack_default_audio_sample_t *in = (jack_default_audio_sample_t *) jack_port_get_buffer (handle->input_port, nframes);
    
    if (buffer_size <= 0) {
        fprintf(stderr, "JACK: buffer is full. Deactivating JACK client.\n");
        return 1;
    }

    /* Write to jack ringbuffer which should be thread safe */
    jack_ringbuffer_write (handle->rbuffer, (char*) in, sample_size * nframes);

#ifdef MIC_SPEAKER_PASSTHROUGH_DEBUG

    jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer (handle->output_port, nframes);

    /* Output mic output to speakers (Just for testing) */
    memcpy (out, in, sample_size * nframes);

#endif

    return 0;      
}

void
error (const char *desc)
{
    fprintf (stderr, "JACK error: %s\n", desc);
}

void
jack_shutdown (void *arg)
{
    exit(1);
}

int
srate (jack_nframes_t nframes, void *arg)

{
    printf ("JACK: The sample rate is now %u/sec\n", nframes);
    return 0;
}

ad_rec_t *
ad_open_dev(const char *dev, int32 sps)
{
    ad_rec_t *handle;
    const char **ports;

#ifdef HAVE_SAMPLERATE_H
    int resample_error;
    double samplerates_ratio;
    jack_nframes_t jack_samplerate;
#endif

    if (dev == NULL) {
        dev = DEFAULT_DEVICE;
    }

    printf("JACK: Setting default device: %s\n", dev);

    if ((handle = (ad_rec_t *) calloc(1, sizeof(ad_rec_t))) == NULL) {
        fprintf(stderr, "calloc(%d) failed\n", (int)sizeof(ad_rec_t));
        abort();
    }
 
    /* Tell the JACK server to call error() whenever it
       experiences an error.  
    */
    jack_set_error_function (error);

    /* Try to become a client of the JACK server */
    if ((handle->client = jack_client_open ("jack_ad", (jack_options_t)0, NULL)) == 0) {
	fprintf (stderr, "jack server not running?\n");
	return NULL;
    }

#ifdef HAVE_SAMPLERATE_H
    handle->resample_buffer = malloc(BUFFER_SIZE);

    jack_samplerate = jack_get_sample_rate(handle->client);
    samplerates_ratio = (double)((double)jack_samplerate / (double)sps);
    
    handle->rbuffer = jack_ringbuffer_create((int)((double)BUFFER_SIZE * samplerates_ratio));
    handle->sample_buffer = malloc((int)((double)BUFFER_SIZE * samplerates_ratio));
#else
    handle->rbuffer = jack_ringbuffer_create(BUFFER_SIZE);
    handle->sample_buffer = malloc(BUFFER_SIZE);
#endif

    if(handle->rbuffer == NULL) {
        fprintf (stderr, "Failed to create jack ringbuffer\n");
        return NULL;
    }

    /* Tell the JACK server to call `process()' whenever
       there is work to be done.
    */
    jack_set_process_callback (handle->client, process, handle);

    /* Tell the JACK server to call `srate()' whenever
       the sample rate of the system changes.
    */
    jack_set_sample_rate_callback (handle->client, srate, 0);

    /* Tell the JACK server to call `jack_shutdown()' if
       it ever shuts down, either entirely, or if it
       just decides to stop calling us.
    */
    jack_on_shutdown (handle->client, jack_shutdown, 0);


    /* Create the input port */
    if((handle->input_port = jack_port_register (handle->client, "jack_ad_input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == 0) {
	fprintf (stderr, "cannot register input port!\n");
	return NULL;
    }
   
    if((handle->output_port = jack_port_register (handle->client, "jack_ad_output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsPhysical|JackPortIsOutput, 0)) == 0) {
        fprintf (stderr, "cannot register output port!\n");
        return NULL;
    }
 
    /* Tell the JACK server that we are ready to start */
    if (jack_activate (handle->client)) {
	fprintf (stderr, "cannot activate client");
	return NULL;
    }

    /* Connect the ports. Note: you can't do this before
       the client is activated, because we can't allow
       connections to be made to clients that aren't
       running.
    */

    if ((ports = jack_get_ports (handle->client, dev, NULL, JackPortIsOutput)) == NULL) {
	fprintf(stderr, "Cannot find any physical capture ports\n");
	return NULL;
    }

    if (jack_connect (handle->client, ports[0], jack_port_name (handle->input_port))) {
	fprintf (stderr, "cannot connect input ports\n");
	return NULL;
    }

    free (ports);

#ifdef MIC_SPEAKER_PASSTHROUGH_DEBUG
    int i;
    if ((ports = jack_get_ports (handle->client, "system:playback", NULL,
                               JackPortIsPhysical|JackPortIsInput)) == NULL) {
        fprintf(stderr, "Cannot find any physical playback ports\n");
        return NULL;
    }

    for (i = 0; ports[i] != NULL; i++) {
        if (jack_connect (handle->client, jack_port_name (handle->output_port), ports[i])) {
            fprintf (stderr, "cannot connect output ports\n");
        }
    }

    free (ports);
#endif

#ifdef HAVE_SAMPLERATE_H
    /* Initialize the sample rate converter. */
    if ((handle->resample_state = src_new (SRC_SINC_BEST_QUALITY, 1, &resample_error)) == NULL) {
        fprintf (stderr, "Error : src_new() failed: %s\n", src_strerror(resample_error));
        return NULL;
    }
#endif

    handle->recording = 0;
    handle->sps = sps;
    handle->bps = sizeof(int16);

    /* Give the jack process callback time to run ? */
    sleep (1);

    return (ad_rec_t *) handle;
}

ad_rec_t *
ad_open_sps(int32 sps)
{
    /* Ignored the sps has to set for the jackd server */
    return ad_open_dev(DEFAULT_DEVICE, sps);
}

ad_rec_t *
ad_open(void)
{
    return ad_open_sps(DEFAULT_SAMPLES_PER_SEC);
}

int32
ad_close(ad_rec_t * handle)
{
    free (handle->sample_buffer);
#ifdef HAVE_SAMPLERATE_H
    free (handle->resample_buffer);
#endif
    jack_ringbuffer_free (handle->rbuffer); 	
    jack_client_close (handle->client);
    free(handle);

    return 0;
}

int32
ad_start_rec(ad_rec_t * handle)
{
    if (handle->recording)
        return AD_ERR_GEN;

    handle->recording = 1;

    return 0;
}

int32
ad_stop_rec(ad_rec_t * handle)
{
    handle->recording = 0;

    return 0;
}

int32
ad_read(ad_rec_t * handle, int16 * buf, int32 max)
{
    int i;
#ifdef HAVE_SAMPLERATE_H
    int resample_error;
#endif

   if (!handle->recording)
       return AD_EOF;

   size_t length = sample_size * max;

#ifdef HAVE_SAMPLERATE_H

   /* Resample the data from the sample rate set in the jack server to that required 
    * by sphinx */

   length = jack_ringbuffer_peek (handle->rbuffer, (char*) handle->sample_buffer, length);
   size_t length_in_samples = length / sample_size;

   if(handle->resample_state == NULL)
       return AD_EOF;

   /* We will try to downsample if jackd is running at a higher sample rate */
   jack_nframes_t jack_samplerate = jack_get_sample_rate(handle->client);

   SRC_DATA data;

   data.data_in = handle->sample_buffer;
   data.input_frames = length_in_samples;
   data.data_out = handle->resample_buffer;
   data.output_frames = BUFFER_SIZE / sample_size;
   data.src_ratio = (float) handle->sps / jack_samplerate;
   data.end_of_input = 0;

   if ((resample_error = src_process(handle->resample_state, &data)) != 0) {
       fprintf (stderr, "resample error %s\n", src_strerror (resample_error));
       return 1;
   }

   for(i = 0; i < data.output_frames_gen; i++) {
       buf[i] = (int16) (int16_range_over_two * (handle->resample_buffer[i] + 1.0) + SHRT_MIN);
   }

   jack_ringbuffer_read_advance(handle->rbuffer, data.input_frames_used * sample_size);	

   if(length == 0 && (!handle->recording)) {
       return AD_EOF;
   }

   return data.output_frames_gen;

#else

   length = jack_ringbuffer_read (handle->rbuffer, (char*) handle->sample_buffer, length);
   size_t length_in_samples = length / sample_size;

   for(i = 0; i < length_in_samples; i++) {
       buf[i] = (int16) (int16_range_over_two * (handle->sample_buffer[i] + 1.0) + SHRT_MIN);
   }

   if(length == 0 && (!handle->recording)) {
       return AD_EOF;
   }

   return length_in_samples;

#endif
}

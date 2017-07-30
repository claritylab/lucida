/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2017 Kamal Galrani <<singularity@lucida.homelinuxserver.org>>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-asrplugin
 *
 * Generic speech to text converter for Lucida AI
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * GST_PLUGIN_PATH=. gst-launch-1.0 --gst-debug="asrplugin:5" \
 * -q filesrc location=sample.wav ! decodebin ! audioconvert ! \
 * audioresample ! asrplugin ! filesink location=output.txt
 * ]|
 * </refsect2>
 */

#include <config.h>
#include <gst/gst.h>
#include "gstasrplugin.h"

#include <jansson.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include <glib-object.h>

#include <thrift/c_glib/protocol/thrift_binary_protocol.h>
#include <thrift/c_glib/transport/thrift_buffered_transport.h>
#include <thrift/c_glib/transport/thrift_socket.h>
#include "a_s_r_thrift_service.h"

/* JSON_REAL_PRECISION is a macro from libjansson 2.7. Ubuntu 12.04 only has 2.2.1-1 */
#ifndef JSON_REAL_PRECISION
#define JSON_REAL_PRECISION(n)  (((n) & 0x1F) << 11)
#endif // JSON_REAL_PRECISION

GST_DEBUG_CATEGORY_STATIC (gst_asrplugin_debug);
#define GST_CAT_DEFAULT gst_asrplugin_debug

/* Filter signals and args */
enum
{
  INTERIM_RESULT_SIGNAL,
  FINAL_RESULT_SIGNAL,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_DECODER_EXECUTABLE,
  PROP_DECODER_CONFIGURATION,
  PROP_MESSAGE_CONTEXT
};

/*
 * the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, format = (string) S16LE, channels = (int) 1, rate = (int) 16000")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("text/x-raw, format= { utf8 }")
    );

static guint gst_asrplugin_signals[LAST_SIGNAL];

#define gst_asrplugin_parent_class parent_class
G_DEFINE_TYPE (Gstasrplugin, gst_asrplugin, GST_TYPE_ELEMENT);

static void gst_asrplugin_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);

static void gst_asrplugin_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_asrplugin_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);

static GstFlowReturn gst_asrplugin_sink_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

static gboolean gst_asrplugin_sink_query(GstPad *pad, GstObject * parent, GstQuery * query);

static void gst_asrplugin_finalize (GObject * object);

static void *gst_asrplugin_read_decoder( void *ptr );

/* TODO: Declare other functons too */

/* GObject vmethod implementations */

/* initialize the asrplugin's class */
static void gst_asrplugin_class_init (GstasrpluginClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_asrplugin_set_property;
  gobject_class->get_property = gst_asrplugin_get_property;

  gobject_class->finalize = gst_asrplugin_finalize;

  g_object_class_install_property (gobject_class, PROP_DECODER_EXECUTABLE,
      g_param_spec_string ("decoder_executable", "Decoder Executable", "Path to decoder executable either absolute or relative to speechrecognition/decoder directory.",
          "", G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_DECODER_CONFIGURATION,
      g_param_spec_string ("decoder_configuration", "Decoder Configuration", "I will forward this as it is to control channel of the decoder. Any consequences are your own doing ;).",
          "{}", G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_MESSAGE_CONTEXT,
      g_param_spec_string ("message_context", "Message Context", "Context of the message. This typically will contain user name and decoder specific context received during last request.",
          "{}", G_PARAM_WRITABLE));

  gst_asrplugin_signals[INTERIM_RESULT_SIGNAL] = g_signal_new(
      "interim-result", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET(GstasrpluginClass, interim_result),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_FLOAT);

  gst_asrplugin_signals[FINAL_RESULT_SIGNAL] = g_signal_new(
      "final-result", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET(GstasrpluginClass, final_result),
      NULL, NULL, NULL, G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_STRING);

  gst_element_class_set_details_simple (gstelement_class,
    "asrplugin",
    "Speech/Audio",
    "Generic speech to text converter for Lucida AI",
    "Kamal Galrani <<singularity@lucida.homelinuxserver.org>>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void gst_asrplugin_init (Gstasrplugin * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, gst_asrplugin_sink_event);
  gst_pad_set_chain_function (filter->sinkpad, gst_asrplugin_sink_chain);
  gst_pad_set_query_function (filter->sinkpad, gst_asrplugin_sink_query);
  gst_pad_use_fixed_caps (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_use_fixed_caps (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->error = NULL;
  filter->decoder_out = NULL;
  filter->segment_length = 0.0;
  filter->shutting_down = false;
  filter->count = 0;
}

/* set_property function
 * Set properties of plugin.
 */
static void gst_asrplugin_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstasrplugin *filter = GST_ASRPLUGIN (object);

  switch (prop_id) {
    case PROP_DECODER_EXECUTABLE:
//      if (G_UNLIKELY (filter->decoder_out != NULL || filter->decoder_ctrl != NULL || filter->decoder_data != -1 ) {
      if (G_UNLIKELY (filter->decoder_out != NULL)) {
        GST_ELEMENT_ERROR (filter, RESOURCE, OPEN_READ_WRITE, (NULL), ("Either a decoder is already loaded or it is in a unrecoverable state!!!"));
        break;
      }

      gchar decoder_command[512];
      FILE *fp;
      guint port;

      fp = popen("./src/get_free_port 2>/dev/null", "r");
      if (fp == NULL) {
        printf("Failed to run com\n" );
        exit(1);
      }
      fscanf(fp, "%u", &port);
      pclose(fp);

      sprintf (decoder_command, "%s --port %u", g_value_get_string (value), port);
      GST_DEBUG_OBJECT (filter, "DECODER: %s\n\n\n", decoder_command);
      filter->decoder_out = popen (decoder_command, "r");

      if (G_UNLIKELY (filter->decoder_out == NULL)) {
          GST_ELEMENT_ERROR (filter, RESOURCE, OPEN_READ, (NULL), ("Could not load decoder!!! Is decoder executable a valid executable file?"));
      }

      sleep(1);

      #if (!GLIB_CHECK_VERSION (2, 36, 0))
          g_type_init ();
      #endif

      filter->socket = g_object_new (THRIFT_TYPE_SOCKET, "hostname",  "localhost", "port", port, NULL);
      filter->transport = g_object_new (THRIFT_TYPE_BUFFERED_TRANSPORT, "transport", filter->socket, NULL);
      filter->protocol  = g_object_new (THRIFT_TYPE_BINARY_PROTOCOL, "transport", filter->transport, NULL);
      thrift_transport_open (filter->transport, &(filter->error));

      if (filter->error) {
          GST_ELEMENT_ERROR (filter, RESOURCE, OPEN_WRITE, (NULL), ("Could not initiate thrift transport to decoder!!! %s", filter->error->message));
          g_clear_error (&(filter->error));
      }

      filter->client = g_object_new (TYPE_A_S_R_THRIFT_SERVICE_CLIENT, "input_protocol",  filter->protocol, "output_protocol", filter->protocol, NULL);

      if (G_UNLIKELY (pthread_create(&filter->tid, NULL, gst_asrplugin_read_decoder, (void *) filter) != 0)) {
          GST_ELEMENT_ERROR (filter, RESOURCE, OPEN_WRITE, (NULL), ("Could not load decoder!!! Failed to start read decoder thread"));
      }

      GST_DEBUG_OBJECT (filter, "Successfully loaded decoder: %s", g_value_get_string (value));
      break;
    case PROP_DECODER_CONFIGURATION:
//      if (G_UNLIKELY (filter->decoder_data != -1)) {
//        GST_ELEMENT_WARNING (filter, RESOURCE, BUSY, (NULL), ("Configuration load requested when decoder is running!!! Ignoring request..."));
//        break;
//      }
      if (G_UNLIKELY (filter->decoder_out == NULL)) {
        GST_ELEMENT_ERROR (filter, RESOURCE, NOT_FOUND, (NULL), ("Configuration load requested before decoder was loaded!!!"));
        break;
      }
      gboolean success;
      a_s_r_thrift_service_if_conf(filter->client, &success, g_value_get_string (value), &(filter->error));
      GST_DEBUG_OBJECT (filter, "Successfully pushed configuration to decoder");
      break;
    case PROP_MESSAGE_CONTEXT:
//      if (G_UNLIKELY (filter->decoder_data != -1)) {
//        GST_ELEMENT_WARNING (filter, RESOURCE, BUSY, (NULL), ("Context load requested when decoder is running!!! Ignoring request..."));
//        break;
//      }
      if (G_UNLIKELY (filter->decoder_out == NULL)) {
        GST_ELEMENT_ERROR (filter, RESOURCE, NOT_FOUND, (NULL), ("Context load requested before decoder was loaded!!!"));
        break;
      }
      a_s_r_thrift_service_if_context(filter->client, g_value_get_string (value), &(filter->error));
      GST_DEBUG_OBJECT (filter, "Successfully pushed context to decoder");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* get_property function
 * Get properties of plugin.
 */
static void gst_asrplugin_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstasrplugin *filter = GST_ASRPLUGIN (object);
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

/* stop_decoder function
 * Stop decoder by closing data FIFO
 */
static gboolean gst_asrplugin_stop_decoder (Gstasrplugin * filter)
{
  GST_DEBUG_OBJECT (filter, "Sending stop command to decoder...");
  if (G_UNLIKELY (filter->decoder_out == NULL)) {
    GST_ELEMENT_ERROR (filter, RESOURCE, NOT_FOUND, (NULL), ("Decoder stop requested before it was loaded!!!"));
    return false;
  }
//  if (G_UNLIKELY (filter->decoder_data == -1)) {
//    GST_ELEMENT_WARNING (filter, RESOURCE, CLOSE, (NULL), ("Decoder stop requested when it is not running!!! Ignoring request..."));
//    gst_pad_push_event(filter->srcpad, gst_event_new_eos());
//    return true;
//  }
  a_s_r_thrift_service_if_stop(filter->client, &(filter->error));
  filter->segment_length = 0;
  return true;
}

/* start_decoder function
 * Send start command to decoder control channel
 */
static gboolean gst_asrplugin_start_decoder (Gstasrplugin * filter)
{
  GST_DEBUG_OBJECT (filter, "Sending start command to decoder...");
  if (G_UNLIKELY (filter->decoder_out == NULL)) {
    GST_ELEMENT_ERROR (filter, RESOURCE, NOT_FOUND, (NULL), ("Decoder start requested before it was loaded!!!"));
    return false;
  }
//  if (G_UNLIKELY (filter->decoder_data != -1)) {
//    GST_ELEMENT_WARNING (filter, RESOURCE, BUSY, (NULL), ("Decoder start requested when it is already running!!! Ignoring request..."));
//    return true;
//  }
  a_s_r_thrift_service_if_start(filter->client, &(filter->error));
  filter->segment_length = 0;
  GST_DEBUG_OBJECT (filter, "Succesfully started decoder");
  return true;
}

static void *gst_asrplugin_read_decoder( void *ptr )
{
  Gstasrplugin *filter =  (Gstasrplugin *) ptr;
  GST_DEBUG_OBJECT (filter, "Starting decoder listener thread...");
  gchar * line = NULL;
  size_t dummy = 0;
  ssize_t bytes;

  while ( ! filter->shutting_down ) {
    bytes = getline(&line, &dummy, filter->decoder_out);
    if (G_UNLIKELY (bytes == -1)) {
      GST_ELEMENT_ERROR (filter, RESOURCE, READ, (NULL), ("Decoder crashed!!! Cleaning up..."));
      // TODO: Clean up, send EOS, quit worker
      break;
    }

    json_t *root;
    json_t *event;
    json_t *status;
    json_t *data;
    json_t *duration;
    json_t *context;
    json_error_t error;
    root = json_loads (line, 0, &error);
    if ( !root ) {
      GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("Received invalid JSON message from decoder!!! Ignoring message..."));
      continue;
    }
    event = json_object_get (root, "event");
    status = json_object_get (root, "status");
    data = json_object_get (root, "data");
    context = json_object_get (root, "context");
    duration = json_object_get (root, "duration");
    if ( !json_is_string (event) ) {
      GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("Received invalid JSON message from decoder: No field 'event' found. Ignoring message..."));
      json_decref(root);
      continue;
    }
    if ( !json_is_integer (status) ) {
      GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("Received invalid JSON message from decoder: No field 'status' found. Ignoring message..."));
      json_decref(root);
      continue;
    }
    if ( !json_is_string (data) ) {
      GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("Received invalid JSON message from decoder: No field 'data' found. Ignoring message..."));
      json_decref(root);
      continue;
    }

    if (strcmp(json_string_value (event), "interim_result") == 0) {
      GST_DEBUG_OBJECT (filter, "Interim results received from decoder");
      gfloat duration_value = 0.0;
      if ( !json_is_real (duration) ) {
        GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("No field 'duration' found while parsing interim results. Using 0.0..."));
      } else {
        duration_value = json_real_value (duration);
      }
      g_signal_emit (filter, gst_asrplugin_signals[INTERIM_RESULT_SIGNAL], 0, json_string_value (data), duration_value);
    } else if (strcmp(json_string_value (event), "final_result") == 0) {
      GST_DEBUG_OBJECT (filter, "Final results received from decoder");

      GstBuffer *buffer = gst_buffer_new_and_alloc (strlen (json_string_value (data)) + 1);
      gst_buffer_fill (buffer, 0, json_string_value (data), strlen (json_string_value (data)));
      gst_buffer_memset (buffer, strlen (json_string_value (data)) , '\n', 1);
      gst_pad_push(filter->srcpad, buffer);

      gfloat duration_value = 0.0;
      if ( !json_is_real (duration) ) {
        GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("No field 'duration' found while parsing final results. Using 0.0..."));
      } else {
        duration_value = json_real_value (duration);
      }
      if ( !json_is_string (context) ) {
        GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("No field 'context' found while parsing interim results. Using empty context..."));
        g_signal_emit (filter, gst_asrplugin_signals[FINAL_RESULT_SIGNAL], 0, json_string_value (data), duration_value, "{}");
      } else {
        g_signal_emit (filter, gst_asrplugin_signals[FINAL_RESULT_SIGNAL], 0, json_string_value (data), duration_value, json_string_value (context));
      }
      gst_pad_push_event(filter->srcpad, gst_event_new_eos());
    } else if (strcmp(json_string_value (event), "eos") == 0) {
      GST_DEBUG_OBJECT (filter, "End-of-Stream received from decoder");
      gst_pad_push_event(filter->srcpad, gst_event_new_eos());
    } else if (strcmp(json_string_value (event), "error") == 0) {
      GST_ELEMENT_ERROR (filter, STREAM, DECODE, (NULL), ("DECODER: %s", json_string_value (data)));
    } else if (strcmp(json_string_value (event), "warn") == 0) {
      GST_ELEMENT_WARNING (filter, STREAM, DECODE, (NULL), ("DECODER: %s", json_string_value (data)));
    } else if (strcmp(json_string_value (event), "info") == 0) {
      GST_ELEMENT_INFO (filter, STREAM, DECODE, (NULL), ("DECODER: %s", json_string_value (data)));
    } else if (strcmp(json_string_value (event), "debug") == 0) {
      GST_DEBUG_OBJECT (filter, "DECODER: %s", json_string_value (data));
    }
    json_decref(root);
  }

  g_free(line);
  GST_DEBUG_OBJECT (filter, "Terminating decoder listener thread...");
  return NULL;
}


/* GstElement vmethod implementations */

/* sink_query function
 * this function handles sink queries
 */
static gboolean gst_asrplugin_sink_query (GstPad *pad, GstObject * parent, GstQuery * query)
{
  gboolean ret;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
      ret = TRUE;
      GstCaps *new_caps = gst_caps_new_simple ("audio/x-raw",
            "format", G_TYPE_STRING, "S16LE",
            "rate", G_TYPE_INT, 16000,
            "channels", G_TYPE_INT, 1, NULL);

      gst_query_set_caps_result (query, new_caps);
      gst_caps_unref (new_caps);
      break;
    default:
      ret = gst_pad_query_default (pad, parent, query);
      break;
  }
  return ret;
}

/* sink_event function
 * this function handles sink events
 */
static gboolean gst_asrplugin_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstasrplugin *filter;
  filter = GST_ASRPLUGIN (parent);

  GST_DEBUG_OBJECT(filter, "Handling %s event", GST_EVENT_TYPE_NAME(event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
      return gst_asrplugin_start_decoder (filter);
    case GST_EVENT_EOS:
      return gst_asrplugin_stop_decoder (filter);
    default:
      return gst_pad_event_default (pad, parent, event);
  }
}

/* sink_chain function
 * this function does the actual processing
 */
static GstFlowReturn gst_asrplugin_sink_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  Gstasrplugin *filter;
  filter = GST_ASRPLUGIN (parent);

  GstMapInfo map;
  if (G_UNLIKELY (gst_buffer_map (buf, &map, GST_MAP_READ) == false)) {
    goto memory_map_issue;
  }
  if (G_UNLIKELY (filter->decoder_out == NULL)) {
    goto decoder_not_loaded;
  }
//  if (G_UNLIKELY (filter->decoder_data == -1)) {
//    goto decoder_not_started;
//  }
  GST_DEBUG_OBJECT (filter, "Pushing chunk to decoder... %ld", map.size);

//  guchar data[29];
//  sprintf ((char*)data, "%02dABCDEFGHIJKLMNOPQRSTUVWXYZ", filter->count);
//  filter->count = filter->count + 1;
//  guint8 data[5];
//  data[0] = 0;
//  data[1] = 1;
//  data[2] = 2;
//  data[3] = 3;
//  data[4] = 4;
  GByteArray* chunk = g_byte_array_new_take (map.data, map.size);
  FILE *fp;
  fp = fopen("/home/singularity/out.bin","ab");
  fwrite (map.data,map.size,1,fp);
  fflush (fp);
  fclose (fp);

  a_s_r_thrift_service_if_push(filter->client, chunk, &(filter->error));
  filter->segment_length = filter->segment_length + (float) map.size / 32000.0;

  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);
  GST_DEBUG_OBJECT (filter, "Pushed chunk to decoder... %ld", map.size);
  return GST_FLOW_OK;

  /* special cases */
  memory_map_issue: {
    GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("Error while reading data from buffer!!! Ignoring chunk..."));

    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return GST_FLOW_OK;
  }
  decoder_not_started: {
    GST_ELEMENT_ERROR (filter, CORE, PAD, (NULL), ("Data recieved before decoder was started!!!"));

    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return GST_FLOW_EOS;
  }
  decoder_not_loaded: {
    GST_ELEMENT_ERROR (filter, CORE, PAD, (NULL), ("Data recieved before decoder was loaded!!!"));

    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
}

static void gst_asrplugin_finalize (GObject * object)
{
  Gstasrplugin *filter;
  filter = GST_ASRPLUGIN (object);

  GST_DEBUG_OBJECT (filter, "Shutting down ASR Plugin...");
  filter->shutting_down = true;

  GST_DEBUG_OBJECT (filter, "Stopping decoder if it is running...");
//  if (G_UNLIKELY (filter->decoder_data != -1)) {
//    gst_asrplugin_stop_decoder (filter);
//  }

  GST_DEBUG_OBJECT (filter, "Waiting for decoder read thread to join...");
//  fclose(filter->decoder_ctrl);
//  filter->decoder_ctrl = NULL;
  pthread_join (filter->tid, NULL);

  GST_DEBUG_OBJECT (filter, "Shutting down decoder subprocess...");
  pclose(filter->decoder_out);
  filter->decoder_out = NULL;

  thrift_transport_close (filter->transport, NULL);

  g_object_unref (filter->client);
  g_object_unref (filter->protocol);
  g_object_unref (filter->transport);
  g_object_unref (filter->socket);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean asrplugin_init (GstPlugin * asrplugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_asrplugin_debug, "asrplugin", 0, DESCRIPTION);

  return gst_element_register (asrplugin, "asrplugin", GST_RANK_NONE, GST_TYPE_ASRPLUGIN);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "asrplugin"
#endif

/* gstreamer looks for this structure to register asrplugins
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    asrplugin,
    "Generic GST plugin for speech recognition in Lucida AI",
    asrplugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)

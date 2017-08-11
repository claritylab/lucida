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
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

#include <glib-object.h>

#include <thrift/c_glib/protocol/thrift_binary_protocol.h>
#include <thrift/c_glib/transport/thrift_buffered_transport.h>
#include <thrift/c_glib/transport/thrift_socket.h>
#include "a_s_r_thrift_service.h"

#include <defs.h>

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
  PROP_REQUEST_ID,
  PROP_LUCIDA_USER,
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

//static void gst_asrplugin_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

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

  gobject_class->finalize = gst_asrplugin_finalize;

  g_object_class_install_property (gobject_class, PROP_DECODER_EXECUTABLE,
      g_param_spec_string ("decoder_executable", "Decoder Executable", "Path to decoder executable either absolute or relative to speechrecognition/decoder directory.",
          "", G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_REQUEST_ID,
      g_param_spec_string ("request_id", "Request ID", "Identifier of the request. Should be unique at least for the clear interval set in speech recognition settings.",
          "", G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_LUCIDA_USER,
      g_param_spec_string ("lucida_user", "Lucida user", "User name of the user stored in Lucida database. This is required for some decoders optional for others.",
          "", G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_MESSAGE_CONTEXT,
      g_param_spec_string ("message_context", "Message Context", "Context of the message. This typically will contain user name and decoder specific context received during last request.",
          "", G_PARAM_WRITABLE));

  gst_asrplugin_signals[INTERIM_RESULT_SIGNAL] = g_signal_new(
      "interim-result", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET(GstasrpluginClass, interim_result),
      NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);

  gst_asrplugin_signals[FINAL_RESULT_SIGNAL] = g_signal_new(
      "final-result", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET(GstasrpluginClass, final_result),
      NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);

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
  g_strlcpy(filter->request_id, "00000000-0000-0000-0000-000000000000", 64);
  g_strlcpy(filter->decoder_executable, "", 512);
}

/* handle_error function
 * Forwards errors and does the needful
 */
static void GST_ASRPLUGIN_HANDLE_ERROR(Gstasrplugin * filter, int type, const gchar* message, ...)
{
  gchar buffer[1024];
  va_list args;
  vsnprintf(buffer, sizeof(buffer), message, args);
  va_end(args);
  GST_ERROR_OBJECT (filter, "%s", buffer);
  GST_ERROR_OBJECT (filter, "%d", type);

  json_t *root = json_object();

  json_object_set_new( root, "error", json_string (buffer) );
  json_object_set_new( root, "type", json_integer (type));

  g_signal_emit (filter, gst_asrplugin_signals[FINAL_RESULT_SIGNAL], 0, json_dumps(root, 0));
  json_decref(root);

  gst_pad_push_event(filter->srcpad, gst_event_new_eos());
}

/* load_decoder function
 * Loadss decoder
 */
static void gst_asrplugin_load_decoder(Gstasrplugin * filter, const gchar* executable)
{
  gchar decoder_command[512];
  FILE *fp;
  guint port;
  fp = popen("./src/get_free_port 2>/dev/null", "r");
  if (fp == NULL || fscanf(fp, "%u", &port) != 1 ) {
    GST_ASRPLUGIN_HANDLE_ERROR(filter, TRY_AGAIN, "I couldn't find a free port to run decoder on!!! Please try again later...");
    return;
  }
  pclose(fp);

  sprintf (decoder_command, "%s --port %u", executable, port);

  GST_DEBUG_OBJECT (filter, "Loading decoder with command: %s", decoder_command);
  filter->decoder_out = popen (decoder_command, "r");

  if (G_UNLIKELY (filter->decoder_out == NULL)) {
    GST_ASRPLUGIN_HANDLE_ERROR(filter, FATAL, "Something went wrong while loading decoder!!! Please check the log...");
    return;
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
    GST_ASRPLUGIN_HANDLE_ERROR(filter, FATAL, "Could not initiate thrift transport to decoder!!! Please check the log...");
    g_clear_error (&(filter->error));
    pclose (filter->decoder_out);
    filter->decoder_out = NULL;
    return;
  }

  filter->client = g_object_new (TYPE_A_S_R_THRIFT_SERVICE_CLIENT, "input_protocol",  filter->protocol, "output_protocol", filter->protocol, NULL);

  filter->shutting_down = false;
  if (G_UNLIKELY (pthread_create(&filter->tid, NULL, gst_asrplugin_read_decoder, (void *) filter) != 0)) {
    GST_ASRPLUGIN_HANDLE_ERROR(filter, FATAL, "Something went wrong while trying to start decoder read thread!!! Please check the log...");
    filter->shutting_down = true;
    pclose(filter->decoder_out);
    filter->decoder_out = NULL;
    return;
  }

  GST_DEBUG_OBJECT (filter, "Successfully loaded decoder: %s", executable);
  g_strlcpy(filter->decoder_executable, executable, 512);
}

/* unload_decoder function
 * Unloads decoder
 */
static void gst_asrplugin_unload_decoder(Gstasrplugin * filter)
{
  filter->shutting_down = true;

  GST_DEBUG_OBJECT (filter, "Stopping decoder if it is running...");
  a_s_r_thrift_service_if_abort(filter->client, &(filter->error));

  GST_DEBUG_OBJECT (filter, "Closing thrift transports...");
  thrift_transport_close (filter->transport, NULL);

  g_object_unref (filter->client);
  g_object_unref (filter->protocol);
  g_object_unref (filter->transport);
  g_object_unref (filter->socket);

  GST_DEBUG_OBJECT (filter, "Waiting for decoder read thread to join...");
  pthread_join (filter->tid, NULL);

  GST_DEBUG_OBJECT (filter, "Shutting down decoder subprocess...");
  pclose(filter->decoder_out);
  filter->decoder_out = NULL;
}

/* set_property function
 * Set properties of plugin.
 */
static void gst_asrplugin_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstasrplugin *filter = GST_ASRPLUGIN (object);

  switch (prop_id) {
    case PROP_DECODER_EXECUTABLE:
      if (G_UNLIKELY (filter->decoder_out != NULL)) {
        GST_WARNING_OBJECT (filter, "A decoder is already loaded while trying to load decoder!!! I'll try to unload existing decoder...");
        gst_asrplugin_unload_decoder(filter);
      }
      gst_asrplugin_load_decoder(filter, g_value_get_string (value));
      break;
    case PROP_REQUEST_ID:
      if (G_UNLIKELY (filter->decoder_out == NULL)) {
        GST_ASRPLUGIN_HANDLE_ERROR(filter, NOT_IN_ORDER, "Setting request identifier before loading decoder makes no sense!!! Trying to restart worker...");
        break;
      }
      a_s_r_thrift_service_if_request_id(filter->client, g_value_get_string (value), &(filter->error));
      g_strlcpy(filter->request_id, g_value_get_string (value), 64);
      break;
    case PROP_LUCIDA_USER:
      if (G_UNLIKELY (filter->decoder_out == NULL)) {
        GST_ASRPLUGIN_HANDLE_ERROR(filter, NOT_IN_ORDER, "Setting user details before loading decoder makes no sense!!! Trying to restart worker...");
        break;
      }
      a_s_r_thrift_service_if_user(filter->client, g_value_get_string (value), &(filter->error));
      break;
    case PROP_MESSAGE_CONTEXT:
      if (G_UNLIKELY (filter->decoder_out == NULL)) {
        GST_ASRPLUGIN_HANDLE_ERROR(filter, NOT_IN_ORDER, "Setting message context before loading decoder makes no sense!!! Trying to restart worker...");
        break;
      }
      a_s_r_thrift_service_if_context(filter->client, g_value_get_string (value), &(filter->error));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* stop_decoder function
 * Stop decoder by closing data FIFO
 */
static gboolean gst_asrplugin_stop_decoder (Gstasrplugin * filter)
{
  if (G_UNLIKELY (filter->decoder_out == NULL)) {
    GST_ASRPLUGIN_HANDLE_ERROR(filter, NOT_IN_ORDER, "Stopping decoder before loading it makes no sense!!! Trying to restart worker...");
    return false;
  }
  GST_DEBUG_OBJECT (filter, "%s: Sending stop command to decoder...", filter->request_id);
  a_s_r_thrift_service_if_stop(filter->client, &(filter->error));
  filter->segment_length = 0;
  return true;
}

/* start_decoder function
 * Send start command to decoder control channel
 */
static gboolean gst_asrplugin_start_decoder (Gstasrplugin * filter)
{
  if (G_UNLIKELY (filter->decoder_out == NULL)) {
    GST_ASRPLUGIN_HANDLE_ERROR(filter, NOT_IN_ORDER, "Starting decoder before loading it makes no sense!!! Trying to restart worker...");
    return false;
  }
  GST_DEBUG_OBJECT (filter, "%s: Sending start command to decoder...", filter->request_id);
  a_s_r_thrift_service_if_start(filter->client, &(filter->error));
  filter->segment_length = 0;
  return true;
}

static void *gst_asrplugin_read_decoder( void *ptr )
{
  Gstasrplugin *filter =  (Gstasrplugin *) ptr;
  GST_DEBUG_OBJECT (filter, "%s: Starting decoder listener thread...", filter->request_id);
  gchar * line = NULL;
  size_t dummy = 0;
  ssize_t bytes;

  while ( ! filter->shutting_down ) {
    bytes = getline(&line, &dummy, filter->decoder_out);
    if (G_UNLIKELY (bytes == -1)) {
      GST_ASRPLUGIN_HANDLE_ERROR(filter, FATAL, "Decoder crashed while processing data!!! Trying to restart worker...");
      break;
    }

    json_t *root;
    json_t *event;
    json_t *status;
    json_t *data;
    json_error_t error;
    root = json_loads (line, 0, &error);
    if ( !root ) {
      GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("%s: Received invalid JSON message from decoder!!! Ignoring message...", filter->request_id));
      continue;
    }
    event = json_object_get (root, "event");
    status = json_object_get (root, "status");
    data = json_object_get (root, "data");
    if ( !json_is_string (event) ) {
      GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("%s: Received invalid JSON message from decoder: No field 'event' found. Ignoring message...", filter->request_id));
      json_decref(root);
      continue;
    }
    if ( !json_is_integer (status) ) {
      GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("%s: Received invalid JSON message from decoder: No field 'status' found. Ignoring message...", filter->request_id));
      json_decref(root);
      continue;
    }
    if ( !json_is_string (data) ) {
      GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("%s: Received invalid JSON message from decoder: No field 'data' found. Ignoring message...", filter->request_id));
      json_decref(root);
      continue;
    }

    if (strcmp(json_string_value (event), "interim_result") == 0) {
      GST_DEBUG_OBJECT (filter, "%s: Interim results received from decoder", filter->request_id);
      g_signal_emit (filter, gst_asrplugin_signals[INTERIM_RESULT_SIGNAL], 0, json_string_value (data));
    } else if (strcmp(json_string_value (event), "final_result") == 0) {
      GST_DEBUG_OBJECT (filter, "%s: Final results received from decoder", filter->request_id);

      GstBuffer *buffer = gst_buffer_new_and_alloc (strlen (json_string_value (data)) + 1);
      gst_buffer_fill (buffer, 0, json_string_value (data), strlen (json_string_value (data)));
      gst_buffer_memset (buffer, strlen (json_string_value (data)) , '\n', 1);
      gst_pad_push(filter->srcpad, buffer);

      g_signal_emit (filter, gst_asrplugin_signals[FINAL_RESULT_SIGNAL], 0, json_string_value (data));
      gst_pad_push_event(filter->srcpad, gst_event_new_eos());
    } else if (strcmp(json_string_value (event), "eos") == 0) {
      GST_DEBUG_OBJECT (filter, "%s: End of stream received from decoder", filter->request_id);
      g_strlcpy(filter->request_id, "00000000-0000-0000-0000-000000000000", 64);
      gst_pad_push_event(filter->srcpad, gst_event_new_eos());
    } else if (strcmp(json_string_value (event), "error") == 0) {
      GST_ASRPLUGIN_HANDLE_ERROR(filter, json_integer_value (status), "%s: %s", filter->request_id, json_string_value (data));
    } else if (strcmp(json_string_value (event), "warn") == 0) {
      GST_WARNING_OBJECT (filter, "%s: %s", filter->request_id, json_string_value (data));
    } else if (strcmp(json_string_value (event), "info") == 0) {
      GST_INFO_OBJECT (filter, "%s: %s", filter->request_id, json_string_value (data));
    } else if (strcmp(json_string_value (event), "debug") == 0) {
      GST_DEBUG_OBJECT (filter, "%s: %s", filter->request_id, json_string_value (data));
    }
    json_decref(root);
  }

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

  GST_DEBUG_OBJECT(filter, "%s: Handling %s event", filter->request_id, GST_EVENT_TYPE_NAME(event));

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
  GST_DEBUG_OBJECT (filter, "%s: Pushing chunk to decoder... %ld", filter->request_id, map.size);

  GByteArray* chunk = g_byte_array_new_take (map.data, map.size);

  a_s_r_thrift_service_if_push(filter->client, chunk, &(filter->error));
  filter->segment_length = filter->segment_length + (float) map.size / 32000.0;

  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  GST_DEBUG_OBJECT (filter, "%s: Pushed chunk to decoder... %ld", filter->request_id, map.size);
  return GST_FLOW_OK;

  /* special cases */
  memory_map_issue: {
    GST_ELEMENT_WARNING (filter, RESOURCE, READ, (NULL), ("%s: Error while reading data from buffer!!! Ignoring chunk...", filter->request_id));

    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return GST_FLOW_OK;
  }
  decoder_not_loaded: {
    GST_ASRPLUGIN_HANDLE_ERROR(filter, FATAL, "Data recieved before decoder was loaded!!! Trying to restart worker...");

    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
}

static void gst_asrplugin_finalize (GObject * object)
{
  Gstasrplugin *filter;
  filter = GST_ASRPLUGIN (object);

  GST_WARNING_OBJECT (filter, "Shutting down ASR Plugin...");
  gst_asrplugin_unload_decoder(filter);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

void G_GNUC_NO_INSTRUMENT gst_lucida_logger (GstDebugCategory *category, GstDebugLevel level, const gchar *file, const gchar *function, gint line, GObject *object, GstDebugMessage *message, gpointer user_data) {
  time_t t = time(NULL);
  struct tm tm = * localtime (&t);
  if (level <= gst_debug_category_get_threshold (category)) {
    gchar* level_str;
    if (level == GST_LEVEL_ERROR) {
      level_str = "\033[31m  ERROR";
    } else if (level == GST_LEVEL_WARNING) {
      level_str = "\033[33m   WARN";
    } else if (level == GST_LEVEL_FIXME) {
      level_str = "\033[43m  FIXME";
    } else if (level == GST_LEVEL_INFO) {
      level_str = "\033[36m   INFO";
    } else if (level == GST_LEVEL_DEBUG) {
      level_str = "\033[37m  DEBUG";
    } else if (level == GST_LEVEL_LOG) {
      level_str = "    LOG";
    } else if (level == GST_LEVEL_TRACE) {
      level_str = "  TRACE";
    } else if (level == GST_LEVEL_MEMDUMP) {
      level_str = "MEMDUMP";
    }
    gchar category_str[32];
    gchar* category_ptr;
    g_strlcpy(category_str, category->name, 32);
    category_ptr = &category_str[0];
    while (*category_ptr) {
      *category_ptr = toupper((unsigned char) *category_ptr);
      category_ptr++;
    }
    printf ("%04d-%02d-%02d %02d:%02d:%02d - \033[1m%7s\033[0m: \033[1m%10s\033[0m: %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, level_str, category_str, gst_debug_message_get(message));
  }
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean asrplugin_init (GstPlugin * asrplugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_asrplugin_debug, "asrplugin", 0, DESCRIPTION);
  gst_debug_remove_log_function (gst_debug_log_default);
  gst_debug_add_log_function(&gst_lucida_logger, NULL, NULL);
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

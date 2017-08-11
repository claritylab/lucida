import os, sys
import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst
GObject.threads_init()
Gst.init(None)

import logging
import thread
import subprocess

logger = logging.getLogger("PIPELINE")

class DecoderPipeline(object):
    def __init__(self, conf):
        logger.info("Initialising %s speech to text decoder" % (conf['decoder']))

        self.interim_result_handler = None
        self.final_result_handler = None
        self.eos_handler = None
        self.error_handler = None

        self.silence_threshold = conf['silence_threshold']
        self.silence_timeout = conf['silence_timeout']
        self.silence_timeout_diff = conf['silence_timeout'] - conf['initial_silence_timeout']
        self.silent_for = self.silence_timeout_diff
        self.data_directory = "/tmp/lucida/speech"

        self.request_id = "00000000-0000-0000-0000-000000000000"

        self._create_pipeline(conf)

    def _create_pipeline(self, conf):
        Gst.Registry.get().scan_path(os.getcwd() + "/src/gstplugin/src")
        Gst.debug_set_threshold_from_string(conf['gstreamer_verbosity'], True)

        self.asr = Gst.ElementFactory.make("asrplugin", "asr")
        self.appsrc = Gst.ElementFactory.make("appsrc", "appsrc")
        self.decodebin = Gst.ElementFactory.make("decodebin", "decodebin")
        self.audioconvert = Gst.ElementFactory.make("audioconvert", "audioconvert")
        self.audioresample = Gst.ElementFactory.make("audioresample", "audioresample")
        self.tee = Gst.ElementFactory.make("tee", "tee")
        self.queue1 = Gst.ElementFactory.make("queue", "queue1")
        self.level = Gst.ElementFactory.make("level", "level")
        self.audiosink = Gst.ElementFactory.make("filesink", "audiosink")
        self.queue2 = Gst.ElementFactory.make("queue", "queue2")
        self.datasink = Gst.ElementFactory.make("fakesink", "datasink")

        self.asr.set_property("decoder_executable", os.getcwd() + "/decoders/" + conf['decoder'] + "/decoder")

        self.level.set_property("post-messages", True)
        self.appsrc.set_property("is-live", True)
        self.audiosink.set_property("location", "/dev/null")

        logger.info("Created GStreamer elements")

        self.pipeline = Gst.Pipeline()
        for element in [self.appsrc, self.decodebin, self.audioconvert, self.audioresample, self.tee, self.queue1, self.level, self.audiosink, self.queue2, self.asr, self.datasink]:
            logger.debug("Adding element %s to the pipeline" % element.__class__.__name__)
            self.pipeline.add(element)

        logger.info("Linking GStreamer elements")

        self.appsrc.link(self.decodebin)
        self.decodebin.connect('pad-added', self._connect_audio_converter)
        self.audioconvert.link(self.audioresample)
        self.audioresample.link(self.tee)

        self.tee.link(self.queue1)
        self.queue1.link(self.level)
        self.level.link(self.audiosink)

        self.tee.link(self.queue2)
        self.queue2.link(self.asr)
        self.asr.link(self.datasink)

        # Create bus and connect several handlers
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()
        self.bus.enable_sync_message_emission()
        self.bus.connect('message::eos', self._on_eos)
        self.bus.connect('message::error', self._on_error)
        self.bus.connect('sync-message::element', self._on_sync_message)

        self.asr.connect('interim-result', self._on_interim_result)
        self.asr.connect('final-result', self._on_final_result)

        logger.info("Setting pipeline to READY")
        self.pipeline.set_state(Gst.State.READY)
        logger.info("Set pipeline to READY")

    def _connect_audio_converter(self, element, pad):
        logger.info("%s: Connecting audio converter" % self.request_id)
        pad.link(self.audioconvert.get_static_pad("sink"))
        logger.info("%s: Connected audio converter" % self.request_id)

    def _on_eos(self, bus, msg):
        self.silent_for = 0.0
        logger.info("%s: Received EOS signal from decoder" % self.request_id)
        self.finish_request()
        if self.eos_handler:
            self.eos_handler()

    def _on_error(self, bus, msg):
        error = msg.parse_error()
        logger.error(error)
        self.cancel()
        if self.error_handler:
            self.error_handler(error[0].message)

    def _on_sync_message(self, bus, msg):
        if msg.get_structure() is None:
            return
        if msg.get_structure().get_name() == 'level':
            if msg.get_structure().get_value("rms")[0] < self.silence_threshold:
                self.silent_for = self.silent_for + 0.1
            elif self.silent_for != self.silence_timeout_diff:
                self.silent_for = 0.0
            if self.silent_for > self.silence_timeout:
                logger.info("%s: Silent for more than %s seconds!!! Pushing EOS to pipeline..." % (self.request_id, str(self.silence_timeout)))
                self.appsrc.emit("end-of-stream")
            logger.info("%s: Silent for %s seconds...." % (self.request_id, str(self.silent_for)))

    def _on_interim_result(self, asr, hyp):
        logger.debug("%s: Received interim result from decoder" % (self.request_id))
        if self.interim_result_handler:
            self.interim_result_handler(hyp)

    def _on_final_result(self, asr, hyp):
        logger.debug("%s: Received final result from decoder %s" % (self.request_id, hyp))
        if self.final_result_handler:
            self.final_result_handler(hyp)

    def finish_request(self):
        logger.info("%s: Resetting decoder state" % self.request_id)
        self.audiosink.set_state(Gst.State.NULL)
        self.audiosink.set_property("location", "/dev/null")
        self.audiosink.set_state(Gst.State.PLAYING)
        self.pipeline.set_state(Gst.State.NULL)
        self.request_id = "00000000-0000-0000-0000-000000000000"
        self.silent_for = 0.0

    def init_request(self, id, caps_str, user, context=""):
        self.pipeline.set_state(Gst.State.PAUSED)
        self.request_id = id
        self.silent_for = self.silence_timeout_diff
        logger.info("%s: Initializing request" % (self.request_id))
        if caps_str and len(caps_str) > 0:
            logger.info("%s: Setting caps to %s" % (self.request_id, caps_str))
            caps = Gst.caps_from_string(caps_str)
            self.appsrc.set_property("caps", caps)
        else:
            self.appsrc.set_property("caps", None)

        self.asr.set_property("request_id", self.request_id)
        self.asr.set_property("lucida_user", user)
        self.asr.set_property("message_context", context)

        self.audiosink.set_state(Gst.State.NULL)
        self.audiosink.set_property('location', "%s/%s.raw" % (self.data_directory, id))
        self.audiosink.set_state(Gst.State.PLAYING)

        self.pipeline.set_state(Gst.State.PLAYING)
        self.audiosink.set_state(Gst.State.PLAYING)

    def process_data(self, data):
        logger.debug('%s: Pushing buffer of size %d to decoder' % (self.request_id, len(data)))
        buf = Gst.Buffer.new_allocate(None, len(data), None)
        buf.fill(0, data)
        self.appsrc.emit("push-buffer", buf)

    def end_request(self):
        logger.info("%s: Pushing EOS to pipeline" % self.request_id)
        self.appsrc.emit("end-of-stream")
        self.silent_for = 0.0

    def set_interim_result_handler(self, handler):
        self.interim_result_handler = handler

    def set_final_result_handler(self, handler):
        self.final_result_handler = handler

    def set_eos_handler(self, handler):
        self.eos_handler = handler

    def set_error_handler(self, handler):
        self.error_handler = handler

    def cancel(self):
        self.end_request()
        id = self.request_id
        self.finish_request()
        try:
            os.remove("%s/%s.raw" % (self.data_directory, id))
        except:
            pass

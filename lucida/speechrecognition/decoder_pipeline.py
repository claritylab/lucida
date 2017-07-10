import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst
GObject.threads_init()
Gst.init(None)

import os
import logging
import thread
import subprocess

os.chdir(os.path.dirname(os.path.realpath(__file__)))
os.environ['GST_PLUGIN_PATH'] = os.path.dirname(os.path.realpath(__file__)) + "/plugin/src"

logger = logging.getLogger(__name__)

class DecoderPipeline(object):
    def __init__(self, conf):
        logger.info("Initialising %s speech to text decoder" % (conf['decoder']))

        self.interim_result_handler = None
        self.final_result_handler = None
        self.eos_handler = None
        self.error_handler = None

        self.request_id = "<undefined>"

        self._create_pipeline(conf)

    def _create_pipeline(self, conf):
        conf = self._load_configuration(conf)
        self.appsrc = Gst.ElementFactory.make("appsrc", "appsrc")
        self.decodebin = Gst.ElementFactory.make("decodebin", "decodebin")
        self.audioconvert = Gst.ElementFactory.make("audioconvert", "audioconvert")
        self.audioresample = Gst.ElementFactory.make("audioresample", "audioresample")
        self.tee = Gst.ElementFactory.make("tee", "tee")
        self.audiosink = Gst.ElementFactory.make("filesink", "audiosink")
        self.asr = Gst.ElementFactory.make("asrplugin", "asr")
        self.datasink = Gst.ElementFactory.make("filesink", "datasink")

        self.asr.set_property("decoder_executable", conf['decoder_executable'])
        self.asr.set_property("decoder_configuration", conf['decoder_configuration'])
#        self.asr.set_property("silence_threshold", conf['silence_threshold'])
        self.asr.set_property("silence_threshold", 2000)
        self.asr.set_property("silence_timeout", conf['silence_timeout'])
        self.asr.set_property("initial_silence_timeout", conf['initial_silence_timeout'])

        self.appsrc.set_property("is-live", True)
        self.audiosink.set_property("location", "/dev/null")
        self.datasink.set_property("location", "/dev/null")

        self.data_directory = conf['data_directory']

        logger.info('Created GStreamer elements')

        self.pipeline = Gst.Pipeline()
        for element in [self.appsrc, self.decodebin, self.audioconvert, self.audioresample, self.tee, self.audiosink, self.asr, self.datasink]:
            logger.debug("Adding %s to the pipeline" % element)
            self.pipeline.add(element)

        logger.info('Linking GStreamer elements')

        self.appsrc.link(self.decodebin)
        self.decodebin.connect('pad-added', self._connect_audio_converter)
        self.audioconvert.link(self.audioresample)
        self.audioresample.link(self.tee)

        self.tee.link(self.audiosink)
        self.tee.link(self.asr)

        self.asr.link(self.datasink)

        # Create bus and connect several handlers
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()
        self.bus.enable_sync_message_emission()
        self.bus.connect('message::eos', self._on_eos)
        self.bus.connect('message::error', self._on_error)

        self.asr.connect('interim-result', self._on_interim_result)
        self.asr.connect('final-result', self._on_final_result)

        logger.info("Setting pipeline to READY")
        self.pipeline.set_state(Gst.State.READY)
        logger.info("Set pipeline to READY")

    def _load_configuration(self, conf):
        conf['decoder_executable'] = os.path.dirname(os.path.realpath(__file__)) + "/decoders/" + conf['decoder'] + "/decoder"
        conf['decoder_configuration'] = subprocess.check_output([os.path.dirname(os.path.realpath(__file__)) + "/decoders/" + conf['decoder'] + "/configure"])
        return conf

    def _connect_audio_converter(self, element, pad):
        logger.info("%s: Connecting audio converter" % self.request_id)
        pad.link(self.audioconvert.get_static_pad("sink"))

    def _on_eos(self, bus, msg):
        logger.info("%s: Received EOS signal from decoder" % self.request_id)
        self.finish_request()
        if self.eos_handler:
            self.eos_handler()

    def _on_error(self, bus, msg):
        error = msg.parse_error()
        logger.error(error)
        self.finish_request()
        if self.error_handler:
            self.error_handler(error[0].message)

    def _on_interim_result(self, asr, hyp, duration):
        logger.info("%s: Received interim result from decoder" % (self.request_id))
        if self.interim_result_handler:
            self.interim_result_handler(hyp, duration)

    def _on_final_result(self, asr, hyp, duration, context):
        logger.info("%s: Received final result from decoder %s" % (self.request_id, hyp))
        if self.final_result_handler:
            self.final_result_handler(hyp, duration, context)

    def finish_request(self):
        logger.info("%s: Resetting decoder state" % self.request_id)
        self.audiosink.set_state(Gst.State.NULL)
        self.audiosink.set_property("location", "/dev/null")
        self.audiosink.set_state(Gst.State.PAUSED)
        self.datasink.set_state(Gst.State.NULL)
        self.datasink.set_property("location", "/dev/null")
        self.datasink.set_state(Gst.State.PAUSED)
        self.pipeline.set_state(Gst.State.PAUSED)
        self.request_id = "<undefined>"

    def init_request(self, id, caps_str, context="{}"):
        self.pipeline.set_state(Gst.State.PAUSED)
        self.request_id = id
        logger.info("%s: Initializing request" % (self.request_id))
        if caps_str and len(caps_str) > 0:
            logger.info("%s: Setting caps to %s" % (self.request_id, caps_str))
            caps = Gst.caps_from_string(caps_str)
            self.appsrc.set_property("caps", caps)
        else:
            self.appsrc.set_property("caps", None)

        self.asr.set_property("message_context", context)

        self.audiosink.set_state(Gst.State.NULL)
        self.audiosink.set_property('location', "%s/%s.raw" % (self.data_directory, id))
        self.audiosink.set_state(Gst.State.PAUSED)

        self.datasink.set_state(Gst.State.NULL)
        self.datasink.set_property('location', "%s/%s.transcript" % (self.data_directory, id))
        self.datasink.set_state(Gst.State.PAUSED)

        self.pipeline.set_state(Gst.State.PLAYING)

    def process_data(self, data):
        logger.debug('%s: Pushing buffer of size %d to decoder' % (self.request_id, len(data)))
        buf = Gst.Buffer.new_allocate(None, len(data), None)
        buf.fill(0, data)
        self.appsrc.emit("push-buffer", buf)

    def end_request(self):
        logger.info("%s: Pushing EOS to pipeline" % self.request_id)
        self.appsrc.emit("end-of-stream")

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
            os.remove("%s/%s.transcript" % (self.data_directory, id))
        finally:
            pass

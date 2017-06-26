"""
Created on Jun 14 2017

@author: kamal1210
"""

import logging
import threading
import time
import requests
import json
from audioop import rms

logger = logging.getLogger(__name__)

class DecoderPipeline(object):
    STATE_CREATED = 0
    STATE_INITIALIZED = 2
    STATE_PROCESSING = 3
    STATE_EOS_RECEIVED = 7
    STATE_FINISHED = 100

    def __init__(self, conf={}):
        try:
            self.api_token            = conf["api-token"]
            self.api_host             = conf["api-host"]
            self.silence_threshold    = conf["silence-threshold"]
            self.silence_timeout      = conf["silence-timeout"]
            self.request_timeout      = conf["request-timeout"]
            self.socket_data_timeout  = conf["socket-data-timeout"]
        except Exception as e:
            logger.error("Key '%s' not found in configuration file!!!" % e.message)
            raise KeyError("Key '%s' not found in configuration file!!!" % e.message)

        self.result_handler = None
        self.full_result_handler = None
        self.eos_handler = None
        self.error_handler = None
        self.request_id = "<undefined>"
        self.buffer = []
        self.lock = threading.Lock()
        self.state = self.STATE_CREATED
        self.silent_for = 0.0
        self.last_chunk_yielded = 0
        self.eos_received = 0
        logger.info("Initialized decoder with endpoint: %s" % (self.api_host))

    def is_silent(self, block):
        try:
            rms_value = rms(block, 2)
            return rms_value, rms_value <= self.silence_threshold
        except:
            return -1, True

    def _post_req_ctrl(self):
	self.request_generator = threading.Thread(target=self._post_req_gen)
        self.request_generator.daemon = True
        self.request_generator.start()
        while self.eos_received == 0:
            time.sleep(1)
        while self.state != self.STATE_FINISHED:
            if time.time() - self.eos_received > self.request_timeout:
                self.request_generator.stop()
                logger.error("Request to %s timed out!!!" % self.api_host)
                if self.error_handler:
                    self.error_handler("Request to %s timed out!!!" % self.api_host)
                self.finish_request()
            time.sleep(1)

    def _post_req_gen(self):
        self.last_chunk_yielded = time.time()
        self.state = self.STATE_PROCESSING
        response = requests.post(self.api_host, headers=self.headers, data=self._post_data_gen())
        if response.status_code == 200:
            response_json = json.loads(response.text)
            result = { 'status': 0, 'total-length': 0.0, 'result': { 'hypotheses': [{ 'likelihood': 100.0, 'confidence': 1.0, 'transcript': response_json['_text']}], 'final': True }, 'segment-length': 0.0, 'segment-start': 0.0 }
            if self.full_result_handler:
                self.full_result_handler(json.dumps(result))
        else:
            response_json = json.loads(response.text)
            logger.error(response_json['error'])
            if self.error_handler:
                self.error_handler(response_json['error'])
        self.finish_request()

    def _post_data_gen(self):
        while self.state == self.STATE_PROCESSING:
            self.lock.acquire()
            if self.buffer:
              chunk = self.buffer.pop(0)
              if chunk != 'EOS':
                  yield chunk
                  self.last_chunk_yielded = time.time()
                  amplitude, silent = self.is_silent(chunk)
                  if silent:
                      self.silent_for = self.silent_for + len(chunk)/2.0/16000
                  else:
                      self.silent_for = 0.0
                  logger.debug("%s: Yielding %d bytes of data with rms %d " % (self.request_id, len(chunk), amplitude))
              else:
                  self.state = self.STATE_EOS_RECEIVED
                  logger.info("%s: Received EOS!!! Closing POST buffer.." % self.request_id)
            self.lock.release()
            if ( time.time() - self.last_chunk_yielded ) > self.socket_data_timeout:
                self.state = self.STATE_EOS_RECEIVED
            if self.silent_for > self.silence_timeout:
                self.state = self.STATE_EOS_RECEIVED
        self.eos_received = time.time()

    def finish_request(self):
        logger.info("%s: Resetting decoder state" % self.request_id)
        if self.eos_handler:
            self.eos_handler[0](self.eos_handler[1])
        self.state = self.STATE_FINISHED
        self.request_id = "<undefined>"
        self.buffer = []
        self.silent_for = 0.0
        self.last_chunk_yielded = 0

    def init_request(self, request_id, caps_str):
        self.request_id = request_id
	content_type='audio/raw;encoding=signed-integer;bits=16;rate=16000;endian=little'
        logger.info("%s: Initializing request" % self.request_id)
	logger.info("%s: Setting content_type to %s" % (self.request_id, content_type))
	self.headers = {'Authorization': 'Bearer ' + self.api_token, 'Content-Type': content_type, 'Accept': 'application/json', 'Transfer-Encoding': 'chunked'}
        self.state = self.STATE_INITIALIZED
	self.request_controller = threading.Thread(target=self._post_req_ctrl)
        self.request_controller.daemon = True
        self.request_controller.start()

    def process_data(self, data):
        if self.state == self.STATE_PROCESSING:
            logger.debug('%s: Pushing %d bytes of data to buffer' % (self.request_id, len(data)))
            self.lock.acquire()
            self.buffer.append(data)
            self.lock.release()

    def end_request(self):
        logger.info("%s: Pushing EOS to buffer" % self.request_id)
        self.lock.acquire()
        self.buffer.append('EOS')
        self.lock.release()

    def set_result_handler(self, handler):
        self.result_handler = handler

    def set_full_result_handler(self, handler):
        self.full_result_handler = handler

    def set_eos_handler(self, handler, user_data=None):
        self.eos_handler = (handler, user_data)

    def set_error_handler(self, handler):
        self.error_handler = handler

    def cancel(self):
        self.request_controller.stop()
        self.request_generator.stop()
        self.finish_request()
        logger.info("%s: Cancelled pipeline" % self.request_id)

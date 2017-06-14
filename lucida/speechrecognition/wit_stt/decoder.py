"""
Created on Jun 14 2017

@author: kamal1210
"""
import logging
import threading
import time
import os
import requests
import json

logger = logging.getLogger(__name__)

API_HOST="https://api.wit.ai/speech"

class DecoderPipeline(object):
    STATE_CREATED = 0
    STATE_CONNECTED = 1
    STATE_INITIALIZED = 2
    STATE_PROCESSING = 3
    STATE_EOS_RECEIVED = 7
    STATE_CANCELLING = 8
    STATE_FINISHED = 100

    def __init__(self, conf={}):
        logger.info("Creating decoder using conf: %s" % conf)
        self.token = conf.get("wit-access-token", "")
        self.partial_results = conf.get("partial-results", True)
        self.max_request_rate = conf.get("max-result-rate", 1)
        self.url = API_HOST + "?v=" + conf.get("wit-api-version", "14/06/2017")

        self.result_handler = None
        self.full_result_handler = None
        self.eos_handler = None
        self.error_handler = None
        self.request_id = "<undefined>"
        self.buffer = []
        self.lock = threading.Lock()
        self.state = self.STATE_INITIALIZED
        logger.info("Initialised decoder pipeline with endpoint: %s" % (self.url))

    def _post_req_gen(self):
        print str(self.headers)
        response = requests.post(self.url, headers=self.headers, data=self._post_data_gen())
        if response.status_code == 200:
            response_json = json.loads(response.text)
            result = { 'status': 0, 'total-length': 0.0, 'result': { 'hypotheses': [{ 'likelihood': 100.0, 'confidence': 1.0, 'transcript': response_json['_text']}], 'final': True }, 'segment-length': 0.0, 'segment-start': 0.0 }
            if self.full_result_handler:
                self.full_result_handler(json.dumps(result))
        else:
            response_json = json.loads(response.text)
            logger.log(response_json['error'])
            if self.error_handler:
                self.error_handler(response_json['error'])
        if self.eos_handler:
            self.eos_handler[0](self.eos_handler[1])
        self.finish_request()

    def _post_data_gen(self):
        while self.state == self.STATE_CONNECTED:
            self.lock.acquire()
            if self.buffer:
              chunk = self.buffer.pop(0)
              if chunk != 'EOS':
                  yield chunk
                  self.last_chunk_yielded = time.time()
                  print "Sending over ", len(chunk), " bytes of data"
              else:
                  self.state = self.STATE_EOS_RECEIVED
                  logger.debug("%s: Received EOS!!! Closing POST buffer.." % self.request_id)
            self.lock.release()
#            if time.time() - self.last_decoder_message > self.request_timeout:
#                state = self.STATE_FINISHED
#                break

    def finish_request(self):
        logger.info("%s: Resetting decoder state" % self.request_id)
        self.state = self.STATE_FINISHED
        self.request_id = "<undefined>"

    def init_request(self, id, caps_str):
#	self.user = user-name
#	self.context = user-context
#	self.thread_id = thread-id
#	self.message_id = message-id
        self.request_id = id
        logger.info("%s: Initializing request" % (self.request_id))
	content_type='audio/raw;encoding=signed-integer;bits=16;rate=16000;endian=little'
	logger.info("%s: Setting content_type to %s" % (self.request_id, content_type))
	self.headers = {'Authorization': 'Bearer ' + self.token, 'Content-Type': content_type, 'Accept': 'application/json', 'Transfer-Encoding': 'chunked'}
        self.state = self.STATE_CONNECTED
	thread = threading.Thread(target=self._post_req_gen)
        thread.daemon = True
        thread.start()

    def process_data(self, data):
        logger.debug('%s: Pushing buffer of size %d to pipeline' % (self.request_id, len(data)))
        self.lock.acquire()
        self.buffer.append(data)
        self.lock.release()

    def end_request(self):
        logger.info("%s: Pushing EOS to pipeline" % self.request_id)
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
        logger.info("%s: Sending EOS to pipeline in order to cancel processing" % self.request_id)
        if self.eos_handler:
            self.eos_handler[0](self.eos_handler[1])
        self.finish_request()
        logger.info("%s: Cancelled pipeline" % self.request_id)

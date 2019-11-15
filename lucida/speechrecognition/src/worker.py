import os, sys, time

import logging
import logging.config
import logging.handlers
import thread
import json

from ws4py.client.threadedclient import WebSocketClient
import ws4py.messaging

# TODO: Replace this with defs
import common

sys.path.insert(0, os.getcwd() + "/include")
import defs

logger = logging.getLogger("WORKER")

class SocketHandler(WebSocketClient):
        STATE_CREATED = 0
        STATE_CONNECTED = 1
        STATE_INITIALIZED = 2
        STATE_PROCESSING = 3
        STATE_EOP_RECEIVED = 6
        STATE_EOS_RECEIVED = 7
        STATE_CANCELLING = 8
        STATE_FINISHED = 100

        def __init__(self, master, pipeline, conf):
                WebSocketClient.__init__(self, url=master, heartbeat_freq=10)

                self.pipeline = pipeline

                self.pipeline.set_interim_result_handler(self._on_interim_result)
                self.pipeline.set_final_result_handler(self._on_final_result)
                self.pipeline.set_error_handler(self._on_error)
                self.pipeline.set_eos_handler(self._on_eos)

                self.request_id = "00000000-0000-0000-0000-000000000000"
                self.silence_timeout = conf['silence_timeout']
                self.initial_silence_timeout = conf['initial_silence_timeout']

                self.call_timeout = conf['max_segment_duration']

                self.state = self.STATE_CREATED

        def opened(self):
                logger.info("Opened websocket connection to server")
                self.state = self.STATE_CONNECTED

        def master_timeout(self):
                while self.state in [self.STATE_INITIALIZED, self.STATE_PROCESSING]:
                        if time.time() - self.last_master_message > self.silence_timeout:
                                logger.warning("%s: More than %d seconds since last message from master, pushing EOS to pipeline" % (self.request_id, self.silence_timeout))
                                self.pipeline.end_request()
                                self.state = self.STATE_EOS_RECEIVED
                                event = dict(status=common.WARN_TIMEOUT)
                                try:
                                        self.send(json.dumps(event))
                                except:
                                        logger.warning("%s: Failed to send error event to master" % (self.request_id))
                                return
                        logger.debug("%s: Checking that master hasn't been silent for more than %d seconds" % (self.request_id, self.silence_timeout))
                        time.sleep(1)

        def call_timeout(self):
                id = self.request_id
                time.sleep(self.call_timeout)
                if id == self.request_id:
                    logger.warning("%s: More than %d seconds since call on hold, closing connection" % (self.request_id, self.call_timeout))
                    in_call = False
                    self._on_eos()

        def received_message(self, message):
                logger.debug("%s: Got message from server of type %s" % (self.request_id, str(type(message))))
                self.last_master_message = time.time()

                if self.state == self.STATE_CONNECTED:
                        if isinstance(message, ws4py.messaging.TextMessage):
                            props = json.loads(str(message))
                            self.context = props['context']
                            self.caps_str = props['caps']
                            self.request_id = props['id']
                            self.user = props['user']
                            self.in_call = props['isCall']
                            self.pipeline.init_request(self.request_id, self.caps_str, self.user, self.context)
                            self.state = self.STATE_INITIALIZED
                            self.last_master_message = time.time() + self.initial_silence_timeout - self.silence_timeout
                            thread.start_new_thread(self.master_timeout, ())
                            logger.info("%s: Started master timeout thread" % self.request_id)
                        else:
                            logger.info("%s: Non-text message received while waiting for initialisation!!! Resetting..." % self.request_id)
                            self.finish_request()
                elif message.data == "EOS":
                        if self.state != self.STATE_CANCELLING and self.state != self.STATE_EOS_RECEIVED and self.state != self.STATE_FINISHED:
                                self.pipeline.end_request()
                                self.state = self.STATE_EOS_RECEIVED
                        else:
                                logger.info("%s: Ignoring EOS, worker already in state %d" % (self.request_id, self.state))
                else:
                        if self.state != self.STATE_CANCELLING and self.state != self.STATE_EOS_RECEIVED and self.state != self.STATE_FINISHED:
                                if isinstance(message, ws4py.messaging.BinaryMessage):
                                        self.pipeline.process_data(message.data)
                                        self.state = self.STATE_PROCESSING
                                else:
                                    logger.info("%s: Non-binary message received while waiting for audio!!! Ignoring message..." % (self.request_id))
                        else:
                                logger.info("%s: Ignoring data, worker already in state %d" % (self.request_id, self.state))


        def finish_request(self):
                if self.state == self.STATE_CONNECTED:
                        self.pipeline.finish_request()
                        self.state = self.STATE_FINISHED
                        return
                if self.state == self.STATE_INITIALIZED:
                        self.pipeline.finish_request()
                        self.state = self.STATE_FINISHED
                        return
                if self.state != self.STATE_FINISHED:
                        logger.info("%s: Master disconnected before decoder reached EOS?" % self.request_id)
                        self.state = self.STATE_CANCELLING
                        self.pipeline.cancel()
                        counter = 0
                        while self.state == self.STATE_CANCELLING:
                                counter += 1
                                if counter > 30:
                                        logger.info("%s: Giving up waiting after %d tries" % (self.request_id, counter))
                                        self.state = self.STATE_FINISHED
                                else:
                                        logger.info("%s: Waiting for EOS from decoder" % self.request_id)
                                        time.sleep(1)
                        logger.info("%s: Finished waiting for EOS" % self.request_id)


        def closed(self, code, reason=None):
                logger.debug("%s: Closed websocket connection to server. Cleaning up..." % self.request_id)
                self.finish_request()
                logger.debug("%s: Done cleaning up after websocket connection closed." % self.request_id)

        def _on_interim_result(self, result):
                logger.debug("%s: Received interim result." % (self.request_id))
                data = json.loads(result)
                event = dict(id=self.request_id, status=common.SUCCESS_OK, result=data['result'])
                try:
                        self.send(json.dumps(event))
                except:
                        e = sys.exc_info()[1]
                        logger.warning("%s: Failed to send event to master: %s" % (self.request_id, e))

        def _on_final_result(self, result):
                restart = False
                logger.debug("%s: Received final result" % (self.request_id))
                data = json.loads(result)
                if 'result' in data:
                        data['id'] = self.request_id
                        data['status'] = common.SUCCESS_OK
                        event = data
                elif 'error' in data:
                        logger.error(data['error'])
                        data['error'] = "I could not understand you! Please try again..."
                        if data['type'] == defs.NOT_AUTHORISED:
                                data['error'] = "You are not authorised to use this service!!! Please visit the web interface to authorise speeech recognition"
                        event = dict(status=common.ERROR_GENERIC, message=data['error'])
                        if data['type'] == defs.FATAL or data['type'] == defs.FATAL:
                                restart = True
                                pass
                try:
                        self.send(json.dumps(event))
                except:
                        e = sys.exc_info()[1]
                        logger.warning("%s: Failed to send event to master: %s" % (self.request_id, e))

                if restart:
                        # TODO: Restart Worker
                        pass

        def _on_eos(self, data=None):
                if self.in_call:
                        self.state = self.STATE_CONNECTED
                        thread.start_new_thread(self.call_timeout, ())
                        logger.info("%s: Started call timeout thread" % self.request_id)
                else:
                        self.state = self.STATE_FINISHED
                        self.close()

        def _on_error(self, error):
                self.state = self.STATE_FINISHED
                event = dict(status=common.ERROR_GENERIC, message=error)
                logger.warning("ERROR: %s" % (error))
                try:
                        self.send(json.dumps(event))
                except:
                        e = sys.exc_info()[1]
                        logger.warning("%s: Failed to send event to master: %s" % (self.request_id, e))
                self.close()

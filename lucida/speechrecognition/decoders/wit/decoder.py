"""
Created on Jun 14 2017

@author: kamal1210
"""
from __future__ import print_function

import os, stat, sys, time, threading, json
sys.path.insert(0, "/usr/local/src/lucida/lucida/speechrecognition/include")

import requests, click
import defs

class SpeechDecoder(object):

    def _send_event(self, type, status, data):
        message = {'event': type, 'status': status, 'data': data}
        self.send_event_lock.acquire()
        try:
            print(json.dumps(message))
            sys.stdout.flush()
        except IOError:
            pass
        self.send_event_lock.release()

    def _post_data_gen(self):
        self._send_event("debug", defs.SUCCESS_OK, "Begin data generator %s" % self.audio_src)
        try:
            fifo = open(self.audio_src, "r", 0)
            while True:
                data = fifo.read(4096)
                if len(data) == 0:
                    break
                yield data
        finally:
            fifo.close()
        self.stopped_listening_at = time.time()
        self.state = defs.DECODER_STATE_DECODING
        self._send_event("debug", defs.SUCCESS_OK, "End data generator")

    def _post_req_gen(self):
        self.state = defs.DECODER_STATE_LISTENING
        self._send_event("debug", defs.SUCCESS_OK, "Begin request generator")
        response = requests.post(self.api_host, headers=self.headers, data=self._post_data_gen())
        self._send_event("debug", defs.SUCCESS_OK, "Waiting for response")
        if response.status_code == 200:
            response_json = json.loads(response.text)
            self._send_event("final_result", defs.SUCCESS_OK, response_json['_text'])
        else:
            self._send_event("debug", defs.SUCCESS_OK, response.text)
            response_json = json.loads(response.text)
            self._send_event("error", defs.ERROR_GENERIC, response_json['error'])
        self.state = defs.DECODER_STATE_READY
        self._send_event("debug", defs.SUCCESS_OK, "End request generator")

    def _post_req_ctrl(self):
        self._send_event("debug", defs.SUCCESS_OK, "Begin request control")
        self.request_generator = threading.Thread(target=self._post_req_gen)
        self.request_generator.daemon = True
        self.request_generator.start()
        while self.state == defs.DECODER_STATE_READY:
            time.sleep(1)
        while self.state == defs.DECODER_STATE_LISTENING:
            time.sleep(1)
        while self.state == defs.DECODER_STATE_DECODING:
            if time.time() - self.stopped_listening_at > self.request_timeout:
                self._send_event("error", defs.ERROR_TIMED_OUT, "Request to %s timed out!!!" % self.api_host)
#                self.request_generator.stop()
                self.state = defs.DECODER_STATE_READY
            time.sleep(1)
        self._send_event("debug", defs.SUCCESS_OK, "End request control")

    def conf(self, message):
        if self.state == defs.DECODER_STATE_CREATED or self.state == defs.DECODER_STATE_READY :
            try:
                _api_token       = message["api-token"]
                _api_version     = message["api-version"]
                _request_timeout = int(message["request-timeout"])
            except Exception as e:
                self._send_event("error", defs.ERROR_INVALID_CONFIG, "Required key '%s' not found in configuration message!!!" % e.message)
                return

            response = requests.get("https://api.wit.ai/message?q=hello&v=" + _api_version, headers={'Authorization': 'Bearer ' + _api_token})
            if response.status_code != 200:
                self._send_event("error", defs.ERROR_INVALID_CONFIG, "Invalid credentials for API host '%s'!!!" % _api_host)
                return

            self.headers         = {'Authorization': 'Bearer ' + _api_token, 'Content-Type': 'audio/raw;encoding=signed-integer;bits=16;rate=16000;endian=little', 'Accept': 'application/json', 'Transfer-Encoding': 'chunked'}
            self.api_host        = "https://api.wit.ai/speech?v=" + _api_version
            self.request_timeout = _request_timeout
            self.state           = defs.DECODER_STATE_READY
            return
        self._send_event("warn", defs.WARN_GENERIC, "Configuration message received while decoding!!! Ignoring message...")

    def context(self, message):
        # Handle context
        pass

    def start(self, message):
        self.request_controller = threading.Thread(target=self._post_req_ctrl)
        self.request_controller.daemon = True
        self.request_controller.start()

    def clear(self, message):
#        try:
#            self.request_generator.stop()
#        except:
#            pass
#        try:
#            self.request_controller.stop()
#        except:
#            pass
        self._send_event("warn", defs.WARN_RESET, "Decoder was reset!!! All data cleared")
        self.state = defs.DECODER_STATE_READY

    def __init__(self, ctrl, data):
        self.send_event_lock = threading.Lock()

        if not os.path.exists(ctrl) or not stat.S_ISFIFO(os.stat(ctrl).st_mode):
            self._send_event("error", defs.ERROR_NOT_A_FIFO, "Control source '%s' is not a named pipe!!!" % ctrl)
            sys.exit(1)
        else:
            self.ctrl_src = ctrl

        if not os.path.exists(data) or not stat.S_ISFIFO(os.stat(data).st_mode):
            self._send_event("error", defs.ERROR_NOT_A_FIFO, "Audio source '%s' is not a named pipe!!!" % data)
            sys.exit(1)
        else:
            self.audio_src = data

        self.state = defs.DECODER_STATE_CREATED

    def event_listener(self):
        while True:
            with open(self.ctrl_src, 'r') as fifo:
                line = fifo.readline()
                self._send_event("debug", defs.SUCCESS_OK, "Received %s" % line)
                try:
                    message = json.loads(line)
                    if "command" in message:
                        getattr(self, message["command"])(message)
                except Exception as e:
                    self._send_event("error", defs.ERROR_CONTROL_MESSAGE, "'%s' is not a valid control message!!! %s" % (line, e.message))
            time.sleep(1)

@click.command()
@click.option('--ctrl', '-c', required=True,
              metavar='<ctrl>', help='Path to control FIFO')
@click.option('--data', '-d', required=True,
              metavar='<data>', help='Path to data FIFO')
def main(ctrl, data):
    decoder = SpeechDecoder(ctrl, data)
    decoder.event_listener()

if __name__ == "__main__":
    main()


# Copyright (C) 2017 Kamal Galrani
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os, sys
from subprocess import Popen, PIPE, check_output
import threading
import json
import logging
import time

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

sys.path.append(os.getcwd())
import configuration

sys.path.append(configuration.LUCIDA_ROOT + "/speechrecognition/include/gen-py")
from asrthriftservice import ASRThriftService

DEFAULT_AUDIO_ITER_SIZE = 4096

class STTInterface():

    def read_decoder(self):
        while self.decoder.stdout:
            self.result = None
            line = self.decoder.stdout.readline()
            if not line:
                break
            logging.debug("%s: Received %s" % (self.id, str(line)))
            try:
                data = json.loads(line)
                if not 'event' in data or not 'status' in data or not 'data' in data:
                    raise Exception()
                if data['event'] == 'final_result':
                    try:
                        data = json.loads(data['data'])
                        if 'error' in data:
                            raise Exception(data['error'])
                        self.result = data['result']['hypotheses'][0]['transcript']
                        logging.error("%s: Setting final result to '%s'" % (self.id, self.result))
                    except Exception as e:
                        self.result = "I didn't get you! Can you please repeat that?"
                        logging.error("%s: Some error occured while decoding response data!!!" % (self.id, str(e)))
                    try:
                        self.decoder.terminate()
                    except:
                        pass
                    break
                elif data['event'] == 'error':
                    self.result = "I didn't get you! Can you please repeat that?"
                    logging.error("%s: %s" % (self.id, data['data']))
                    try:
                        self.asr.abort()
                    except:
                        pass
                    try:
                        self.decoder.terminate()
                    except:
                        pass
                    break
                elif data['event'] == 'warn':
                    logging.warn("%s: %s" % (self.id, data['data']))
                elif data['event'] == 'info':
                    logging.info("%s: %s" % (self.id, data['data']))
                elif data['event'] == 'debug':
                    logging.debug("%s: %s" % (self.id, data['data']))
            except Exception as e:
                self.result = "I didn't get you! Can you please repeat that?"
                logging.error("%s: Some error occured while decoding response data!!! Exception: %s" % (self.id, str(e)))
        if not self.result:
            self.result = "I didn't get you! Can you please repeat that?"
        self.completed.set()

    def process(self):
        # Make socket
        tsocket = TSocket.TSocket('localhost', self.port)

        # Buffering is critical. Raw sockets are very slow
        self.transport = TTransport.TBufferedTransport(tsocket)

        # Wrap in a protocol
        protocol = TBinaryProtocol.TBinaryProtocol(self.transport)

        # Create a client to use the protocol encoder
        self.asr = ASRThriftService.Client(protocol)

        with open("/tmp/lucida/speech/%s_out.raw" % self.id, 'rb') as fp:
            self.transport.open()
            self.asr.request_id(self.id)
            self.asr.user(self.user)
            self.asr.start()
            while True:
                data = fp.read(DEFAULT_AUDIO_ITER_SIZE)
                if not data:
                    break
                self.asr.push(data)
            self.asr.stop()
            self.transport.close()
        self.completed.wait()
        count = 0
        while not self.result and count < 10:
            time.sleep(0.1)
            count = count + 1
        return self.result

    def __init__(self, id, user):
        self.completed = threading.Event()
        self.completed.clear()
        self.id = id
        self.user = user

        self.port = int(check_output(['./src/get_free_port', '2>/dev/null']))

        self.decoder = Popen([configuration.LUCIDA_ROOT + "/speechrecognition/decoders/" + configuration.STT_ENGINE + "/decoder", "--port", str(self.port)], stdout=PIPE, cwd=configuration.LUCIDA_ROOT + "/speechrecognition")
        self.decoder_read_thread = threading.Thread(target=self.read_decoder)
        self.decoder_read_thread.daemon = True
        self.decoder_read_thread.start()

    def clean(self):
       try:
           self.asr.abort()
       except:
           pass
       try:
           self.decoder.terminate()
       except:
           pass
       self.completed.set()

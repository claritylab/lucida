#!/usr/bin/env python

# Copyright (C) 2017 Google Inc.
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

from port import PORT

import sys
import os
sys.path.append(os.getcwd())
import configuration

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TServer

from lucidatypes.ttypes import QuerySpec
from lucidaservice import LucidaService

import google_auth_oauthlib.flow
import json

sys.path.append(configuration.LUCIDA_ROOT + "/tts/encoders/" + configuration.TTS_ENGINE)
from encoder import tts

from ga_interface import GAInterface
from stt_interface import STTInterface

import logging
logging.basicConfig(level=logging.DEBUG)

class GATextHandler(LucidaService.Iface):
    def create(self, LUCID, spec):
        """
        Do nothing
        """
        return

    def learn(self, LUCID, knowledge):
        """
        Do nothing
        """
        return

    def infer(self, LUCID, query):
        """
        Determine the weather based off input
        Parameters:
         - LUCID: ID of Lucida user
         - query: query
        """

        input_data = query.content[0].data[-1]
        output_data = "I could not get you! Could you please try again?"
        if os.path.isfile(configuration.CREDENTIALS_DIR + "/" + LUCID + ".json"):
            logging.debug("GA::INFER Query '%s' from user %s" % (str(query), LUCID))
            response = tts(input_data)
            if response['status'] == 'success':
                stt = None
                try:
                    stt = STTInterface(response['message'], LUCID)
                    ga = GAInterface(LUCID)
                    response = ga.converse(response['message'])
                    if 'error' in response:
                        raise Exception(response['error'])
                    output_data = stt.process()
                    logging.debug("GA::INFER Response '/tmp/lucida/speech/%s_out.raw'" % (str(response['request_id'])))
                except Exception as e:
                    logging.error("GA::INFER Some error occured: %s" % str(e))
                try:
                    stt.clean()
                except:
                    pass
        else:
            flow = google_auth_oauthlib.flow.InstalledAppFlow.from_client_secrets_file(configuration.GA_CLIENT_SECRET, scopes=["https://www.googleapis.com/auth/assistant-sdk-prototype"])
            flow.redirect_uri = flow._OOB_REDIRECT_URI
            auth_url, _ = flow.authorization_url(prompt='consent')
            output_data = "Sorry, I don't have permission to access your Google Assistant.\n"
            if input_data.startswith("@GA authorise "):
                try:
                    flow.fetch_token(code=input_data[14:])
                    creds = {
                        'refresh_token': flow.credentials.refresh_token,
                        'token_uri': flow.credentials.token_uri,
                        'client_id': flow.credentials.client_id,
                        'client_secret': flow.credentials.client_secret,
                        'scopes': flow.credentials.scopes
                    }
                    with open(configuration.CREDENTIALS_DIR + "/" + LUCID + ".json", 'w') as outfile:
                        json.dump(creds, outfile)
                    output_data = "You have succesfully connected with Google Assistant :D\n\n"
                    output_data += "In order to use the Google Assistant, you must share certain activity data with Google. The Google Assistant needs this data to function properly\n"
                    output_data += "Open the Activity Controls page (https://myaccount.google.com/activitycontrols) for the Google account that you connected with and ensure that the following toggle switches are enabled (blue):\n"
                    output_data += " * Web & App Activity\n"
                    output_data += "   * In addition, be sure to select the Include Chrome browsing history and activity from websites and apps that use Google services checkbox.\n"
                    output_data += " * Device Information\n"
                    output_data += " * Voice & Audio Activity"
                    return output_data
                except Exception as e:
                    logging.error("GA::AUTHORISE %s" % e)
                    output_data = "Incorrect code!!! Please try again...\n"
            output_data += "Please visit the following url and reply with '@GA authorise <code>'.\n"
            output_data += auth_url
        logging.debug("GA::INFER Response: %s" % output_data)
        return output_data

# Set handler to our implementation
handler = GATextHandler()
processor = LucidaService.Processor(handler)
transport = TSocket.TServerSocket(port=PORT)
tfactory = TTransport.TFramedTransportFactory()
pfactory = TBinaryProtocol.TBinaryProtocolFactory()
server = TServer.TSimpleServer(processor, transport, tfactory, pfactory)

print('GA microservice running on port %d' % PORT)
server.serve()

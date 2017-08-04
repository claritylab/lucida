#!/usr/bin/env python

from port import PORT

import sys
import os.path
import config

import logging
logging.basicConfig(level=logging.DEBUG)

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TServer

from lucidatypes.ttypes import QuerySpec
from lucidaservice import LucidaService

import google_auth_oauthlib.flow

sys.path.append(config.LUCIDA_ROOT + "/lucida/tts/encoders/" + config.TTS_ENGINE)
print sys.path
from encoder import tts

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

        input_data = query.content[0].data[-1][0]
        print input_data
        output_data = "Some error occured!!! Please get in touch with the administrator..."
        if os.path.isfile("../auth/" + LUCID + ".json"):
            print "Input received for infer: %s, %s" % (LUCID, str(query))
        else:
            output_data = "Sorry, I don't have permission to access your Google Assistant.\n"
            if input_data.startswith("@GA authorise "):
                print "trying to auth..."
                try:
                    flow.fetch_token(code=input_data[14:])
                    creds = {
                        'refresh_token': flow.credentials.refresh_token,
                        'token_uri': flow.credentials.token_uri,
                        'client_id': flow.credentials.client_id,
                        'client_secret': flow.credentials.client_secret,
                        'scopes': flow.credentials.scopes
                    }
                    with open("../auth/" + LUCID + ".json", 'w') as outfile:
                        json.dump(creds, outfile)
                    output_data = "You have succesfully connected with Google Assistant :D\n\n"
                    output_data += "In order to use the Google Assistant, you must share certain activity data with Google. The Google Assistant needs this data to function properly\n"
                    output_data += "Open the Activity Controls page (https://myaccount.google.com/activitycontrols) for the Google account that you connected with and ensure that the following toggle switches are enabled (blue):\n"
                    output_data += " * Web & App Activity\n"
                    output_data += "   * In addition, be sure to select the Include Chrome browsing history and activity from websites and apps that use Google services checkbox.\n"
                    output_data += " * Device Information\n"
                    output_data += " * Voice & Audio Activity"
                    return output_data
                except:
                    output_data = "Incorrect code!!! Please try again...\n"
            flow = google_auth_oauthlib.flow.InstalledAppFlow.from_client_secrets_file("../auth/" + config.CLIENT_SECRET, scopes=["https://www.googleapis.com/auth/assistant-sdk-prototype"])
            flow.redirect_uri = flow._OOB_REDIRECT_URI
            auth_url, _ = flow.authorization_url(prompt='consent')
            output_data += "Please visit the following url and reply with '@GA authorise <code>'.\n"
            output_data += auth_url
        return output_data

# Set handler to our implementation
handler = GATextHandler()
processor = LucidaService.Processor(handler)
transport = TSocket.TServerSocket(port=PORT)
tfactory = TTransport.TFramedTransportFactory()
pfactory = TBinaryProtocol.TBinaryProtocolFactory()
server = TServer.TSimpleServer(processor, transport, tfactory, pfactory)

print 'GA microservice running on port %d' % PORT
server.serve()

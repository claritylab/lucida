#!/usr/bin/env python

import sys
sys.path.append('../')

from WeatherConfig import*

from lucidatypes.ttypes import QuerySpec
from lucidaservice import LucidaService

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TServer

import json
import urllib

class WeatherHandler(LucidaService.Iface):
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
        result = 'No weather found for %s' % input_data
        url_location = urllib.quote_plus(input_data)

        # weather from Weather Underground
        try:
            f = urllib.urlopen(WU_API_URL_BASE + \
                    '%s/conditions/q/%s.json' % (WU_API_KEY, url_location))
            json_string = f.read()
            parsed_json = json.loads(json_string)
            if 'error' not in parsed_json['response'] \
                    and 'current_observation' in parsed_json:
                weather = parsed_json['current_observation']['weather']
                temp = parsed_json['current_observation']['temperature_string']
                city = parsed_json['current_observation']['display_location']['full']
                result = "Current weather in %s is %s %s" % (city, weather, temp)
                print 'From Weather Underground: %s' % result
            else:
                # weather from Open Weather Map
                f = urllib.urlopen(OWM_API_URL_BASE + \
                        'q=%s&appid=%s&units=imperial&type=like' % (url_location, OWM_API_KEY))
                json_string = f.read()
                parsed_json = json.loads(json_string)
                if 'weather' in parsed_json and 'main' in parsed_json and 'name' in parsed_json:
                    weather = parsed_json['weather'][0]['main']
                    temp = parsed_json['main']['temp']
                    city = parsed_json['name']
                    if city in input_data:
                        result = 'Current weather in %s is %s, %s F' % (city, weather, temp)
                    print 'From Open Weather Map: %s' % result
            f.close()
        except IOError as err:
            if 401 in err:
                result = 'Unauthorized Weather API keys'
            else:
                result = 'Weather Service is broken!'
        return result

# Set handler to our implementation
handler = WeatherHandler()
processor = LucidaService.Processor(handler)
transport = TSocket.TServerSocket(port=PORT)
tfactory = TTransport.TFramedTransportFactory()
pfactory = TBinaryProtocol.TBinaryProtocolFactory()
server = TServer.TSimpleServer(processor, transport, tfactory, pfactory)

print 'WE at port  %d' % PORT
server.serve()

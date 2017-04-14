#!/usr/bin/env python

port = 8088
owm_api_key = "362537b891e6df03a316e82565fe4df3"
wu_api_key = "ff76e8e80c802d46"

import sys
sys.path.append('../')

from lucidatypes.ttypes import QuerySpec
from lucidaservice import LucidaService

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TServer

import urllib2
import json
import urllib

class WeatherHandler(LucidaService.Iface):
    """
    Do nothing
    Parameters:
     - LUCID: ID of Lucida user
     - spec: spec
    """
    def create(self, LUCID, spec):
        return

    """
    Do nothing
    Parameters:
     - LUCID: ID of Lucida user
     - knowledge: knowledge
    """
    def learn(self, LUCID, knowledge):
        return

    """
    Determine the weather based off input
    Parameters:
     - LUCID: ID of Lucida user
     - query: query
    """
    def infer(self, LUCID, query):
        result = "No weather found for %s" % query.content[0].data[0]
        url_location = urllib.quote_plus(query.content[0].data[0])

        try:
            # weather from Weather Underground
            f = urllib2.urlopen('http://api.wunderground.com/api/%s/conditions/q/%s.json' % (wu_api_key, url_location) )
            json_string = f.read()
            parsed_json = json.loads(json_string)
            if 'error' not in parsed_json['response'] and 'current_observation' in parsed_json:
                weather = parsed_json['current_observation']['weather']
                temp = parsed_json['current_observation']['temperature_string']
                city = parsed_json['current_observation']['display_location']['full']
                result = "Current weather in %s is %s %s" % (city, weather, temp)
                print "From Weather Underground: %s" % result
            else:
                # weather from Open Weather Map
                f = urllib2.urlopen('http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=imperial&type=like' % (url_location, owm_api_key))
                json_string = f.read()
                parsed_json = json.loads(json_string)
                if 'weather' in parsed_json and 'main' in parsed_json and 'name' in parsed_json:
                    weather = parsed_json['weather'][0]['main']
                    temp = parsed_json['main']['temp']
                    city = parsed_json['name']
                    result = "Current weather in %s is %s, %s F" % (city, weather, temp)
                    print "From Open Weather Map: %s" % result
            f.close()
        except urllib2.HTTPError as err:
            if err == "404":
                print result

        return result

# Set handler to our implementation
handler = WeatherHandler()

processor = LucidaService.Processor(handler)
transport = TSocket.TServerSocket(port=port)
tfactory = TTransport.TFramedTransportFactory()
pfactory = TBinaryProtocol.TBinaryProtocolFactory()

server = TServer.TSimpleServer(processor, transport, tfactory, pfactory)

print "WE at port  %d" % port
server.serve()

#!/usr/bin/env python

import requests
import json
import config

def tts(output, text, lang='en'):
    try:
        if len(text.decode('utf-8')) > 100:
            return {'status': 'error', 'message': 'Text interface for Google Assitant currently supports only upto 100 characters. Please split your question or ask your administrator to upgrade to paid TTS engine...'}
        response = requests.post("http://soundoftext.com/sounds", data={'lang': lang, 'text': text}, timeout=( 10, 10))
        data = json.loads(response.text)
        if not data['success']:
            raise Exception()
        response = requests.get("http://soundoftext.com/sounds/" + str(data['id']), timeout=( 10, 10))
        response = requests.get("http://soundoftext.com" + response.text[ (response.text.find("src=\"/static/sounds") + 5) : (response.text.find(".mp3") + 4) ])
        output = output + '.mp3'
        with open(output, 'wb') as f:
            f.write(response.content)
        return {'status': 'success', 'message': output}
    except Exception, e:
        print "Error occured while converting speech to text!!! Please try again later..."
        print str(e)
        return {'status': 'error', 'message': 'An unknown error occured!!! Please try again...'}

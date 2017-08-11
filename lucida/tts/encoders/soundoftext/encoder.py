#!/usr/bin/env python

import logging
import requests
import json
import config
import uuid
from pydub import AudioSegment
from io import BytesIO
import os, errno

def tts(text, lang='en'):
    try:
        try:
            os.makedirs("/tmp/lucida/speech")
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
        if len(text.decode('utf-8')) > 100:
            return {'status': 'error', 'message': 'Text interface for Google Assitant currently supports only upto 100 characters. Please split your question or ask your administrator to upgrade to paid TTS engine...'}
        response = requests.post("http://soundoftext.com/sounds", data={'lang': lang, 'text': text}, timeout=( 10, 10))
        print response.text
        data = json.loads(response.text)
        if not data['success']:
            raise Exception(json.dumps(data))
        response = requests.get("http://soundoftext.com/sounds/" + str(data['id']), timeout=( 10, 10))
        response = requests.get("http://soundoftext.com" + response.text[ (response.text.find("src=\"/static/sounds") + 5) : (response.text.find(".mp3") + 4) ])
        if response.encoding:
            raise Exception("Could not download audio file...")
        id = str(uuid.uuid4())
        output = "/tmp/lucida/speech/" + id + ".raw"
        sound = AudioSegment.from_mp3(BytesIO(response.content))
        sound.export(output, format='s16le', codec='pcm_s16le', bitrate='16k', parameters=['-ac', '1', '-ar', '16k'])
        return {'status': 'success', 'message': id}
    except Exception, e:
        logging.error("Error occured while converting text to speech!!! Please try again later...")
        logging.error(str(e))
        return {'status': 'error', 'message': 'An unknown error occured!!! Please try again...'}

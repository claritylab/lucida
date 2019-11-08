from flask import *
from Database import database
from AccessManagement import login_required
from ThriftClient import thrift_client
from QueryClassifier import query_classifier
from Utilities import log, check_image_extension
from Parser import port_dic
import Config
import os
import json
from flask_socketio import emit
from . import socketio

speech = Blueprint('speech', __name__, template_folder='templates')

@speech.route('/socketio')
def socketio_demo():
	return render_template('socketio.html')

@socketio.on('connect')
def socket_connect():
	emit('stt_status', { 'status': 200, 'engines': ['kaldi'] })

@socketio.on('stt_control')
def stt_control(message):
	print('received control message: ', str(message))
	emit('stt_status', { 'status': 200, 'message': 'Accepted' })

@socketio.on('stt_audio')
def receive_audio(message):
	print('received audio chunk: ', str(message))

@socketio.on('disconnect')
def socket_disconnect():
	print('Client disconnected')

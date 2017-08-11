#!/usr/bin/env python
#
# Copyright 2013 Tanel Alumae

"""
Reads speech data via websocket requests, sends it to Redis, waits for results from Redis and
forwards to client via websocket
"""
import sys
import logging
import json
import codecs
import os.path
import uuid
import time
import threading
import functools
from Queue import Queue

import tornado.ioloop
import tornado.options
import tornado.web
import tornado.websocket
import tornado.gen
import tornado.concurrent
import tornado.wsgi

from Database import database

from Parser import cmd_port

from tornado.options import define
define("port", default=cmd_port, help="run on the given port", type=int)

STATUS_EOS = -1
STATUS_SUCCESS = 0
STATUS_NO_SPEECH = 1
STATUS_ABORTED = 2
STATUS_AUDIO_CAPTURE = 3
STATUS_NETWORK = 4
STATUS_NOT_ALLOWED = 5
STATUS_SERVICE_NOT_ALLOWED = 6
STATUS_BAD_GRAMMAR = 7
STATUS_LANGUAGE_NOT_SUPPORTED = 8
STATUS_NOT_AVAILABLE = 9
STATUS_NOT_AUTHENTICATED = 401
STATUS_CONNECTED = 100

class Application(tornado.web.Application):
	def __init__(self, flaskApp):
		settings = dict(
			cookie_secret="43oETzKXQAGaYdkL5gEmGeJJFuYh7EQnp2XdTP1o/Vo=",
			static_path=os.path.join(os.path.dirname(os.path.dirname(__file__)), "static"),
			xsrf_cookies=False,
			autoescape=None
		)

		flaskWebHandler = tornado.wsgi.WSGIContainer(flaskApp)
		handlers = [
			(r"/ws/speech", DecoderSocketHandler),
			(r"/ws/status", StatusSocketHandler),
			(r"/api/speech", DecoderAPIHandler),
			(r"/worker/ws/speech", WorkerSocketHandler),
			(r"/client/static/(.*)", tornado.web.StaticFileHandler, {'path': settings["static_path"]}),
			(r".*", tornado.web.FallbackHandler, dict(fallback=flaskWebHandler))
		]
		tornado.web.Application.__init__(self, handlers, **settings)
		self.available_workers = set()
		self.status_listeners = set()
		self.num_requests_processed = 0

	def send_status_update_single(self, ws):
		status = dict(num_workers_available=len(self.available_workers), num_requests_processed=self.num_requests_processed)
		ws.write_message(json.dumps(status))

	def send_status_update(self):
		for ws in self.status_listeners:
			self.send_status_update_single(ws)

def run_async(func):
	@functools.wraps(func)
	def async_func(*args, **kwargs):
		func_hl = threading.Thread(target=func, args=args, kwargs=kwargs)
		func_hl.start()
		return func_hl

	return async_func


def content_type_to_caps(content_type):
	"""
	Converts MIME-style raw audio content type specifier to GStreamer CAPS string
	"""
	default_attributes= {"rate": 16000, "format" : "S16LE", "channels" : 1, "layout" : "interleaved"}
	media_type, _, attr_string = content_type.replace(";", ",").partition(",")
	if media_type in ["audio/x-raw", "audio/x-raw-int"]:
		media_type = "audio/x-raw"
		attributes = default_attributes
		for (key,_,value) in [p.partition("=") for p in attr_string.split(",")]:
			attributes[key.strip()] = value.strip()
		return "%s, %s" % (media_type, ", ".join(["%s=%s" % (key, value) for (key,value) in attributes.iteritems()]))
	else:
		return content_type


@tornado.web.stream_request_body
class DecoderAPIHandler(tornado.web.RequestHandler):
	"""
	Provides a HTTP POST/PUT interface supporting chunked transfer requests
	"""

	def prepare(self):
		self.id = str(uuid.uuid4())
		self.final_result = ""
		self.final_result_queue = Queue()
		user = self.request.headers.get("user", None)
		interface = self.request.headers.get("interface", None)
		token = self.request.headers.get("token", None)
                user = database.get_username(interface, user)
                if user == None:
                        self.set_status(401)
                        self.finish("Not Authorized")
			return
		logging.info("API::SPEECH %s: Received request from user '%s' over interface '%s'" % (self.id, user, interface))
		self.worker = None
		self.error_status = 0
		self.error_message = None
		try:
			self.worker = self.application.available_workers.pop()
			self.application.send_status_update()
			logging.info("API::SPEECH %s: Using worker %s" % (self.id, self.__str__()))
			self.worker.set_client_socket(self)
                        context = self.request.headers.get("context", None)
                        content_type = self.request.headers.get("Content-Type", None)
                        if content_type:
                                content_type = content_type_to_caps(content_type)
                                logging.info("API::SPEECH %s: Using content type: %s" % (self.id, content_type))
                        self.worker.write_message(json.dumps(dict(id=self.id, caps=content_type, context=context, user=user, isCall=False)))
		except:
			logging.warn("API::SPEECH %s: No worker available for client request" % self.id)
			self.set_status(503)
			self.finish("No workers available")

	def data_received(self, chunk):
		assert self.worker is not None
		logging.info("API::SPEECH %s: Forwarding client message of length %d to worker" % (self.id, len(chunk)))
		self.worker.write_message(chunk, binary=True)

	def post(self, *args, **kwargs):
		self.end_request(args, kwargs)

	def put(self, *args, **kwargs):
		self.end_request(args, kwargs)

	@run_async
	def get_final_result(self, callback=None):
		logging.info("API::SPEECH %s: Waiting for final result..." % self.id)
		callback(self.final_result_queue.get(block=True))

	@tornado.web.asynchronous
	@tornado.gen.coroutine
	def end_request(self, *args, **kwargs):
		logging.info("API::SPEECH %s: Handling the end of request" % self.id)
		assert self.worker is not None
		self.worker.write_message("EOS", binary=True)
		result = yield tornado.gen.Task(self.get_final_result)
		if self.error_status == 0:
			logging.info("%s: Final result: %s" % (self.id, result))
			self.write(result)
		else:
			logging.info("%s: Error (status=%d) processing HTTP request: %s" % (self.id, self.error_status, self.error_message))
			response = {"status" : self.error_status, "id": self.id, "message": self.error_message}
			self.write(response)
		self.worker.set_client_socket(None)
		self.worker.close()
		self.finish()
		logging.info("API::SPEECH Everything done")

	def send_event(self, event):
		event_str = str(event)
		if len(event_str) > 100:
			event_str = event_str[:97] + "..."
		logging.info("%s: Receiving event %s from worker" % (self.id, event_str))
		if event["status"] == 0 and ("result" in event):
			try:
				if len(event["result"]["hypotheses"]) > 0 and event["result"]["final"]:
					self.final_result = str(event)
			except:
				e = sys.exc_info()[0]
				logging.warn("Failed to extract hypothesis from recognition result:" + e)
		elif event["status"] != 0:
			self.error_status = event["status"]
			self.error_message = event.get("message", "")

	def close(self):
		logging.info("API::SPEECH %s: Receiving close from worker" % (self.id))
		self.final_result_queue.put(self.final_result)
		self.application.send_status_update()

class StatusSocketHandler(tornado.websocket.WebSocketHandler):
        # needed for Tornado 4.0
        def check_origin(self, origin):
                return True

        def open(self):
                logging.debug("Opened a new control channel")
                self.application.status_listeners.add(self)
                self.application.send_status_update_single(self)

        def on_message(self, message):
                assert self.client_socket is not None
                event = json.loads(message)
                self.client_socket.send_event(event)

        def on_close(self):
                logging.info("Status listener left")
                self.application.status_listeners.remove(self)

class WorkerSocketHandler(tornado.websocket.WebSocketHandler):
	def __init__(self, application, request, **kwargs):
		tornado.websocket.WebSocketHandler.__init__(self, application, request, **kwargs)
		self.client_socket = None

	# needed for Tornado 4.0
	def check_origin(self, origin):
		return True

	def open(self):
		self.client_socket = None
		self.application.available_workers.add(self)
		logging.info("New worker available " + self.__str__())
		self.application.send_status_update()

	def on_close(self):
		logging.info("Worker " + self.__str__() + " leaving")
		self.application.available_workers.discard(self)
		if self.client_socket:
			self.client_socket.close()
		self.application.send_status_update()

	def on_message(self, message):
		assert self.client_socket is not None
		event = json.loads(message)
		self.client_socket.send_event(event)
		if 'next_id' in event:
			self.client_socket.set_id(event['id'])

	def set_client_socket(self, client_socket):
		self.client_socket = client_socket


class DecoderSocketHandler(tornado.websocket.WebSocketHandler):
	# needed for Tornado 4.0
	def check_origin(self, origin):
		return True

	def set_id(self, id):
		self.id = id

	def send_event(self, event):
		event["id"] = self.id
		event_str = str(event)
		if len(event_str) > 100:
			event_str = event_str[:97] + "..."
		logging.info("%s: Sending event %s to client" % (self.id, event_str))
		self.write_message(json.dumps(event))

	def open(self):
		self.id = str(uuid.uuid4())
		logging.info("%s: OPEN" % (self.id))
		logging.info("%s: Request arguments: %s" % (self.id, " ".join(["%s=\"%s\"" % (a, self.get_argument(a)) for a in self.request.arguments])))
		user = self.get_argument("user", None, True)
		interface = self.get_argument("interface", None, True)
		user = database.get_username(interface, user)
                if user == None:
			logging.warn("Not authorised")
			event = dict(status=STATUS_NOT_AVAILABLE, message="User not authorised!!!")
			self.send_event(event)
			self.close()
                        return
		self.worker = None
		try:
			self.worker = self.application.available_workers.pop()
			self.application.send_status_update()
			logging.info("%s: Using worker %s" % (self.id, self.__str__()))
			self.worker.set_client_socket(self)

			context = self.get_argument("context", None, True)
			self.isCall = self.get_argument("isCall", False, True)
			content_type = self.get_argument("content-type", None, True)
			if content_type:
				logging.info("%s: Using content type: %s" % (self.id, content_type))

                        self.worker.write_message(json.dumps(dict(id=self.id, caps=content_type, context=context, user=user, isCall=self.isCall)))
		except KeyError:
			logging.warn("%s: No worker available for client request" % self.id)
			event = dict(status=STATUS_NOT_AVAILABLE, message="No decoder available, try again later")
			self.send_event(event)
			self.close()

	def on_connection_close(self):
		logging.info("%s: Handling on_connection_close()" % self.id)
		self.application.num_requests_processed += 1
		self.application.send_status_update()
		if self.worker:
			try:
				self.worker.set_client_socket(None)
				logging.info("%s: Closing worker connection" % self.id)
				self.worker.close()
			except:
				pass

	def on_message(self, message):
		assert self.worker is not None
		logging.info("%s: Forwarding client message (%s) of length %d to worker" % (self.id, type(message), len(message)))
		if isinstance(message, unicode):
			self.worker.write_message(message, binary=False)
		else:
			self.worker.write_message(message, binary=True)

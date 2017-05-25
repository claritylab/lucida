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


class Application(tornado.web.Application):
	def __init__(self):
		settings = dict(
			cookie_secret="43oETzKXQAGaYdkL5gEmGeJJFuYh7EQnp2XdTP1o/Vo=",
			template_path=os.path.join(os.path.dirname(os.path.dirname(__file__)), "templates"),
			static_path=os.path.join(os.path.dirname(os.path.dirname(__file__)), "static"),
			xsrf_cookies=False,
			autoescape=None,
		)

		handlers = [
			(r"/", MainHandler),
			(r"/client/ws/speech", DecoderSocketHandler),
			(r"/client/ws/status", StatusSocketHandler),
			(r"/client/dynamic/reference", ReferenceHandler),
			(r"/client/dynamic/recognize", HttpChunkedRecognizeHandler),
			(r"/worker/ws/speech", WorkerSocketHandler),
			(r"/client/static/(.*)", tornado.web.StaticFileHandler, {'path': settings["static_path"]}),
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

	def save_reference(self, content_id, content):
		refs = {}
		try:
			with open("reference-content.json") as f:
				refs = json.load(f)
		except:
			pass
		refs[content_id] = content
		with open("reference-content.json", "w") as f:
			json.dump(refs, f, indent=2)


class MainHandler(tornado.web.RequestHandler):
	def get(self):
		current_directory = os.path.dirname(os.path.abspath(__file__))
		parent_directory = os.path.join(current_directory, os.pardir)
		readme = os.path.join(parent_directory, "README.md")
		self.render(readme)


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
class HttpChunkedRecognizeHandler(tornado.web.RequestHandler):
	"""
	Provides a HTTP POST/PUT interface supporting chunked transfer requests, similar to that provided by
	http://github.com/alumae/ruby-pocketsphinx-server.
	"""

	def prepare(self):
		self.id = str(uuid.uuid4())
		self.final_hyp = ""
		self.final_result_queue = Queue()
		self.user_id = self.request.headers.get("device-id", "none")
		self.content_id = self.request.headers.get("content-id", "none")
		logging.info("%s: OPEN: user='%s', content='%s'" % (self.id, self.user_id, self.content_id))
		self.worker = None
		self.error_status = 0
		self.error_message = None
		try:
			self.worker = self.application.available_workers.pop()
			self.application.send_status_update()
			logging.info("%s: Using worker %s" % (self.id, self.__str__()))
			self.worker.set_client_socket(self)

			content_type = self.request.headers.get("Content-Type", None)
			if content_type:
				content_type = content_type_to_caps(content_type)
				logging.info("%s: Using content type: %s" % (self.id, content_type))

			self.worker.write_message(json.dumps(dict(id=self.id, content_type=content_type, user_id=self.user_id, content_id=self.content_id)))
		except KeyError:
			logging.warn("%s: No worker available for client request" % self.id)
			self.set_status(503)
			self.finish("No workers available")

	def data_received(self, chunk):
		assert self.worker is not None
		logging.debug("%s: Forwarding client message of length %d to worker" % (self.id, len(chunk)))
		self.worker.write_message(chunk, binary=True)

	def post(self, *args, **kwargs):
		self.end_request(args, kwargs)

	def put(self, *args, **kwargs):
		self.end_request(args, kwargs)

	@run_async
	def get_final_hyp(self, callback=None):
		logging.info("%s: Waiting for final result..." % self.id)
		callback(self.final_result_queue.get(block=True))

	@tornado.web.asynchronous
	@tornado.gen.coroutine
	def end_request(self, *args, **kwargs):
		logging.info("%s: Handling the end of chunked recognize request" % self.id)
		assert self.worker is not None
		self.worker.write_message("EOS", binary=True)
		logging.info("%s: yielding..." % self.id)
		hyp = yield tornado.gen.Task(self.get_final_hyp)
		if self.error_status == 0:
			logging.info("%s: Final hyp: %s" % (self.id, hyp))
			response = {"status" : 0, "id": self.id, "hypotheses": [{"utterance" : hyp}]}
			self.write(response)
		else:
			logging.info("%s: Error (status=%d) processing HTTP request: %s" % (self.id, self.error_status, self.error_message))
			response = {"status" : self.error_status, "id": self.id, "message": self.error_message}
			self.write(response)
		self.application.num_requests_processed += 1
		self.application.send_status_update()
		self.worker.set_client_socket(None)
		self.worker.close()
		self.finish()
		logging.info("Everything done")

	def send_event(self, event):
		event_str = str(event)
		if len(event_str) > 100:
			event_str = event_str[:97] + "..."
		logging.info("%s: Receiving event %s from worker" % (self.id, event_str))
		if event["status"] == 0 and ("result" in event):
			try:
				if len(event["result"]["hypotheses"]) > 0 and event["result"]["final"]:
					if len(self.final_hyp) > 0:
						self.final_hyp += " "
					self.final_hyp += event["result"]["hypotheses"][0]["transcript"]
			except:
				e = sys.exc_info()[0]
				logging.warn("Failed to extract hypothesis from recognition result:" + e)
		elif event["status"] != 0:
			self.error_status = event["status"]
			self.error_message = event.get("message", "")

	def close(self):
		logging.info("%s: Receiving 'close' from worker" % (self.id))
		self.final_result_queue.put(self.final_hyp)


class ReferenceHandler(tornado.web.RequestHandler):
	def post(self, *args, **kwargs):
		content_id = self.request.headers.get("Content-Id")
		if content_id:
			content = codecs.decode(self.request.body, "utf-8")
			user_id = self.request.headers.get("User-Id", "")
			self.application.save_reference(content_id, dict(content=content, user_id=user_id, time=time.strftime("%Y-%m-%dT%H:%M:%S")))
			logging.info("Received reference text for content %s and user %s" % (content_id, user_id))
			self.set_header('Access-Control-Allow-Origin', '*')
		else:
			self.set_status(400)
			self.finish("No Content-Id specified")

	def options(self, *args, **kwargs):
		self.set_header('Access-Control-Allow-Origin', '*')
		self.set_header('Access-Control-Allow-Methods', 'POST, OPTIONS')
		self.set_header('Access-Control-Max-Age', 1000)
		# note that '*' is not valid for Access-Control-Allow-Headers
		self.set_header('Access-Control-Allow-Headers',  'origin, x-csrftoken, content-type, accept, User-Id, Content-Id')


class StatusSocketHandler(tornado.websocket.WebSocketHandler):
	# needed for Tornado 4.0
	def check_origin(self, origin):
		return True

	def open(self):
		logging.info("New status listener")
		self.application.status_listeners.add(self)
		self.application.send_status_update_single(self)

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

	def set_client_socket(self, client_socket):
		self.client_socket = client_socket


class DecoderSocketHandler(tornado.websocket.WebSocketHandler):
	# needed for Tornado 4.0
	def check_origin(self, origin):
		return True

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
		self.user_id = self.get_argument("user-id", "none", True)
		self.content_id = self.get_argument("content-id", "none", True)
		self.worker = None
		try:
			self.worker = self.application.available_workers.pop()
			self.application.send_status_update()
			logging.info("%s: Using worker %s" % (self.id, self.__str__()))
			self.worker.set_client_socket(self)

			content_type = self.get_argument("content-type", None, True)
			if content_type:
				logging.info("%s: Using content type: %s" % (self.id, content_type))

			self.worker.write_message(json.dumps(dict(id=self.id, content_type=content_type, user_id=self.user_id, content_id=self.content_id)))
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


def main():
	logging.basicConfig(level=logging.DEBUG, format="%(levelname)8s %(asctime)s %(message)s ")
	logging.debug('Starting up server')
	from tornado.options import options

	tornado.options.parse_command_line()
	app = Application()
	app.listen(3000)
	tornado.ioloop.IOLoop.instance().start()


if __name__ == "__main__":
	main()

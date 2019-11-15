from __future__ import absolute_import
from __future__ import division
from __future__ import unicode_literals

from controllers.FlaskApp import app
from controllers import WebServer
from controllers.Parser import cmd_port
import logging
import os

def main():
    cmd_host = '0.0.0.0'
    logging.basicConfig(level=logging.DEBUG, format="%(levelname)8s %(asctime)s %(message)s ")
    WebServer.tornado.options.parse_command_line()

    if os.environ.get('SECURE_HOST'):
        logging.info('Spinning up web server at https://' + str(cmd_host)  + ':' + str(cmd_port))
        WebServer.Application(app).listen(cmd_port, address=str(cmd_host), ssl_options={
            "certfile":"certs/server.crt",
            "keyfile":"certs/server.key"})
    else:
        logging.info('Spinning up web server at http://' + str(cmd_host)  + ':' + str(cmd_port))
        WebServer.Application(app).listen(cmd_port, address=str(cmd_host))

    WebServer.tornado.ioloop.IOLoop.instance().start()

if __name__ == '__main__':
    main()

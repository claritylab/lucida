#!/usr/bin/python

import pickledb
import subprocess, re, os, sys
from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from SimpleHTTPServer import SimpleHTTPRequestHandler
from SocketServer import ThreadingMixIn
import threading
import cgi
from datetime import datetime

PORT_NUMBER = 8080
dlog = 'img-log/'

def shcom(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    out = p.communicate()[0]
    return out
#This class will handles any incoming request from
#the browser 
class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        message =  threading.currentThread().getName()
        self.wfile.write(message)
        self.wfile.write('\n')
        return

    def do_POST(self):
        form = cgi.FieldStorage(
            fp=self.rfile,
            headers=self.headers,
            environ={'REQUEST_METHOD':'POST',
                     'CONTENT_TYPE':self.headers['Content-Type'],
                    })

        t = datetime.now()
        filename = dlog + 'input-' + str(t.hour) + str(t.minute) + str(t.second) + '.jpg'
        with open(filename, 'wb') as f:
            f.write(form.value)

        cmd = ('./match --match %s --database matching/attractions/db' % filename)
        res = shcom(cmd)
        # hack
        answer = ('\t%s\n' % os.path.basename(res))
        self.wfile.write(answer)

        return
    
class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in a separate thread."""

if __name__ == '__main__':
    server = ThreadedHTTPServer(('localhost', 8080), Handler)
    print 'Starting server, use <Ctrl-C> to stop'
    server.serve_forever()

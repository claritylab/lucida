#!/usr/bin/python

import pickledb
import subprocess, re, os, sys
from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from SimpleHTTPServer import SimpleHTTPRequestHandler
from SocketServer import ThreadingMixIn
import threading
import cgi
from datetime import datetime

dlog = 'log'

size = 'db'
name = 'landmarks'
img_db = 'matching/%s/%s' % (name, size)
name += '.pickle'
pickdb = pickledb.load(name, True)

def shcmd(cmd):
    subprocess.call(cmd, shell=True)

def shcom(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    out = p.communicate()[0]
    return out

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

        cmd = './match --match %s --database %s' % (filename, img_db)
        res = os.path.basename(shcom(cmd)).strip()
        # hack
        img_data = ' '.join(pickdb.get(res))
        print 'img: %s data: %s' % (res, img_data)
        answer = ('%s\n' % img_data)
        self.wfile.write(answer)

        return
    
class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in a separate thread."""

if __name__ == '__main__':
    cmd = 'mkdir -p ' + dlog
    shcmd(cmd)
    host = sys.argv[1]
    port = int(sys.argv[2])
    server = ThreadedHTTPServer((host, port), Handler)
    print 'Starting server on %s:%s, use <Ctrl-C> to stop' % (host, port)
    server.serve_forever()

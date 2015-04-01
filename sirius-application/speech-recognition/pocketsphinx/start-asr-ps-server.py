#!/usr/bin/env python

import subprocess, re, os, sys
from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from SimpleHTTPServer import SimpleHTTPRequestHandler
from SocketServer import ThreadingMixIn
import threading
import cgi
from datetime import datetime

dlog = '../input-log/'
# these can be used to specify different pocketsphinx configs provided the
# models are available (see ps-args-en.txt and ps-args-vox.txt)
argfiles = ['']

def shcmd(cmd):
    subprocess.call(cmd, shell=True)

def shcom(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    out = p.communicate()[0]
    return out

def shback(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    return p

def shcin(stdin_data):
    server_pid.stdin.write(stdin_data)

    out = server_pid.stdout.readline()
    print out
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
        filename = 'input-' + str(t.hour) + str(t.minute) + str(t.second) + '.wav'
        filepath = dlog + filename
        with open(filepath, 'wb') as f:
            f.write(form.value)

        cmd = 'ffmpeg -y -i %s -acodec pcm_s16le -ac 1 -ar 16000 %s16k_%s 1>/dev/null 2>/dev/null' % (filepath, dlog, filename)
        shcmd(cmd)

        f1 = dlog + "16k_" + filename
        for f in argfiles:
            cmd = './pocketsphinx_continuous %s -logfn /dev/null -infile %s ' % (f, f1)
            res = shcom(cmd).strip()
            print 'transcript [%s]: %s' % (f, res)

        answer = ('%s\n' % res)
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

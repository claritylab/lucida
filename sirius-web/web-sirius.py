#!/usr/bin/env python

#
# Copyright (c) 2015, University of Michigan.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
#
#

import subprocess, re, os, sys, json, urllib, urllib2, httplib, multiprocessing
from datetime import datetime
from httplib import BadStatusLine
from flask import Flask, request, render_template, redirect, url_for
from werkzeug import secure_filename
from OpenSSL import SSL
import time

UPLOAD_FOLDER='input-log/'
ALLOWED_EXTENSIONS=set(['wav'])

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

servers = ['localhost']
QA  = servers[0]
ASR = servers[0]
VIS = servers[0]
ports = [8080, 8081, 8082]

# data folder
log = 'input-log/'

def shcmd(cmd):
    subprocess.call(cmd, shell=True)

def shcom(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    out = p.communicate()[0]
    return out

from wtforms import Form, BooleanField, TextField, PasswordField, validators

def req_vis(image, return_dict=None):
    start = time.time()
    cmd = 'wget -q  -U "Mozilla/5.0" --post-file ' + str(image)
    cmd += ' --header "Content-Type: image/jpeg"'
    cmd += ' -O - '+ make_server(VIS, ports[2])
    result = shcom(cmd)
    if return_dict is not None:
        return_dict[image] = result

    print "VIS time: %f" % float(time.time() - start)
    return result

def req_asr(speech, return_dict=None):
    start = time.time()
    cmd = 'wget -q -U "Mozilla/5.0" --post-file ' + str(speech)
    cmd += ' --header "Content-Type: audio/vnd.wave; rate=16000" -O - '
    cmd += make_server(ASR, ports[1])
    result = shcom(cmd)
    if return_dict is not None:
        return_dict[speech] = result

    print "ASR time: %f" % float(time.time() - start)
    return result

def req_qa(text, return_dict=None):
    start = time.time()
    words = text.split()
    query = ''
    for w in words:
        query += w+'%20'
    if not query:
        return query

    q = make_server(QA, ports[0]) + '?query=' + query

    try:
        resp = urllib2.urlopen(q)
        answer = resp.read()
    except BadStatusLine:
        answer = ''

    print "QA time: %f" % float(time.time() - start)
    print text
    print answer
    return answer

def make_server(ip, port):
    return str('http://' + ip + ':' + str(port))

def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1] in ALLOWED_EXTENSIONS

class RegistrationForm(Form):
    qs = TextField('Type something', [validators.Length(min=2, max=200)])
    in_img = TextField('Link', [validators.Length(min=2, max=200)])

@app.route('/image', methods=['GET','POST'])
def image():
    form = RegistrationForm(request.form)
    if request.method == 'POST':
        f = request.files['file']
        if f and allowed_file(f.filename):
            filename = secure_filename(f.filename)
            f.save(os.path.join(app.config['UPLOAD_FOLDER'], filename))

            speech = 'input-log/' + filename
            t = datetime.now()
            img = log + 'image-' + str(t.month) + str(t.day) + str(t.hour) + str(t.minute) + str(t.second) + '.jpg'
            urllib.urlretrieve(form.in_img.data, img)

            # launch vis, speech in parallel
            manager = multiprocessing.Manager()
            return_dict = manager.dict()

            fns = [req_vis, req_asr]
            data = [img, speech]
            proc = []
            for idx,fn in enumerate(fns):
                p = multiprocessing.Process(target=fn, args=(data[idx],
                                                             return_dict))
                p.start()
                proc.append(p)
            for p in proc:
                p.join()

            question = re.sub('this', return_dict[img].strip(),
                              return_dict[speech].strip()).strip()
            answer = req_qa(question).strip()
            data = [question, answer]
            return render_template('image.html', form=form,
                                   in_img=form.in_img.data,
                                   data=map(json.dumps, data))
    else:
        return render_template('image.html', form=form)

@app.route('/record', methods=['GET','POST'])
def record():
    form = RegistrationForm(request.form)
    if request.method == 'POST':
        f = request.files['file']
        if f and allowed_file(f.filename):
            filename = secure_filename(f.filename)
            f.save(os.path.join(app.config['UPLOAD_FOLDER'], filename))

            filename = 'input-log/' + filename
            question = req_asr(filename).strip()
            answer = req_qa(question).strip()

            data = [question, answer];
            return render_template('record.html', form=form,
                                   data=map(json.dumps, data))
    else:
        return render_template('record.html', form=form)

@app.route('/question', methods=['GET', 'POST'])
def question():
    form = RegistrationForm(request.form)
    if request.method == 'POST':
        text = form.qs.data
        if re.search('.wav', text) and os.path.isfile(text):
            text = req_asr(text)

        answer = req_qa(text)

        line1 = 'Question: %s' % text
        line2 = 'Answer: %s' % answer
        return render_template('question.html', form=form, reply_line1=line1,
                               reply_line2=line2)

    else:
        return render_template('question.html', form=form)

@app.route('/', methods=['GET'])
def index():
    return render_template('index.html')

if __name__ == "__main__":
    cmd = 'mkdir -p ' + log
    shcmd(cmd)
    h = sys.argv[1]
    p = int(sys.argv[2])

    pkey = os.getcwd() + '/server.key'
    cert = os.getcwd() + '/server.crt'
    if os.path.isfile(pkey) and os.path.isfile(cert):
        context = SSL.Context(SSL.SSLv3_METHOD)
        context.use_privatekey_file(pkey)
        context.use_certificate_file(os.getcwd() + '/server.crt')
        app.run(host=h, port=p, debug=True, ssl_context=(cert,
                                                                   pkey) )
    else:
        app.run(host=h, port=p, debug=True)

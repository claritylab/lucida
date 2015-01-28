#!/usr/bin/env python

import subprocess, re, os, sys
import urllib, urllib2, httplib
from datetime import datetime
from httplib import BadStatusLine
import multiprocessing
from flask import Flask, request, render_template, redirect, url_for
from werkzeug import secure_filename
from OpenSSL import SSL

UPLOAD_FOLDER='input-log/'
ALLOWED_EXTENSIONS=set(['wav'])

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

servers = ['141.212.106.240']
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
    cmd = 'wget -q  -U "Mozilla/5.0" --post-file ' + str(image)
    cmd += ' --header "Content-Type: image/jpeg"'
    cmd += ' -O - '+ make_server(VIS, ports[2])
    result = shcom(cmd)
    if return_dict is not None:
        return_dict[image] = result

    return result

def req_asr(speech, return_dict=None):
    # # convert to 16KHz
    # cmd = 'ffmpeg -y -i ' + filename + ' -acodec pcm_s16le -ac 1 -ar 16000 input-log/tmp.wav'
    # shcmd(cmd)
    cmd = 'wget -q -U "Mozilla/5.0" --post-file ' + str(speech)
    cmd += ' --header "Content-Type: audio/vnd.wave; rate=16000" -O - '
    cmd += make_server(ASR, ports[1])
    result = shcom(cmd)
    if return_dict is not None:
        return_dict[speech] = result

    return result

def req_qa(text, return_dict=None):
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
            img = log + 'image-' + str(t.hour) + str(t.minute) + str(t.second) + '.jpg'
            urllib.urlretrieve(form.in_img.data, img)

            # launch vis, speech in parallel
            manager = multiprocessing.Manager()
            return_dict = manager.dict()

            fns = [req_vis, req_asr]
            data = [img, speech]
            proc = []
            for idx,fn in enumerate(fns):
                p = multiprocessing.Process(target=fn, args=(data[idx], return_dict))
                p.start()
                proc.append(p)
            for p in proc:
                p.join()

            text = re.sub('this', return_dict[img], return_dict[speech])
            answer = req_qa(text)
            q = 'Question: %s' % text
            a = 'Answer: %s' % answer
            return render_template('image.html', form=form, in_img=form.in_img.data, question=q, answer=a)
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
            text = req_asr(filename)
            # text = req_asr('input-log/tmp.wav')
            answer = req_qa(text)

            line1 = 'Question: %s' % text
            line2 = 'Answer: %s' % answer
            return render_template('record.html', form=form, reply_line1=line1, reply_line2=line2)
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
        return render_template('question.html', form=form, reply_line1=line1, reply_line2=line2)

    else:
        return render_template('question.html', form=form)

@app.route('/', methods=['GET'])
def index():
    return render_template('index.html')

if __name__ == "__main__":
    cmd = 'mkdir -p ' + log
    shcmd(cmd)
    app.run(host='localhost', port=8000, debug=True)

from flask import *
from AccessManagement import login_required
from ThriftClient import thrift_client
from QueryClassifier import query_classifier
from Utilities import log, check_image_extension
import Config
import os

infer = Blueprint('infer', __name__, template_folder='templates')

@infer.route('/infer', methods=['GET', 'POST'])
@login_required
def infer_route():
    options = {}
    if os.environ.get('ASR_ADDR_PORT'):
        options['asr_addr_port'] = os.environ.get('ASR_ADDR_PORT')
    else:
        options['asr_addr_port'] = 'ws://localhost:8081'
    try:
        form = request.form
        # Deal with POST requests.
        if request.method == 'POST':
            # If the request does not contain an "op" field.
            if not 'op' in form:
                raise RuntimeError('Did you click the Ask button?')
            # When the "op" field is equal to "add_image".
            elif form['op'] == 'infer':
                # Check input file.
                upload_file = request.files['file'] if 'file' in request.files \
                    else None
                if not upload_file is None and upload_file.filename != '':
                    check_image_extension(upload_file)
                # Classify the query.
                speech_input = form['speech_input'] if 'speech_input' in form \
                    else ''
                print '@@@@@@@@@@', speech_input
                image_input = [upload_file.read()] if upload_file else None
                lucida_id = session['username']
                # Check if context is saved for Lucida user
                # If not, classify query, otherwise restore session
                if lucida_id not in Config.SESSION:
                    services_needed = \
                        query_classifier.predict(speech_input, upload_file)
                    speech_input = [speech_input]
                else:
                    services_needed = Config.SESSION[lucida_id]['graph']
                    Config.SESSION[lucida_id]['data']['text'].append(speech_input)
                    speech_input = Config.SESSION[lucida_id]['data']['text']
                options['result'] = thrift_client.infer(lucida_id, \
                    services_needed, speech_input, image_input)
                log('Result ' + options['result'])
                # Check if Calendar service is needed.
                # If so, JavaScript needs to receive the parsed dates.
                if services_needed.has_service('CA'):
                    options['dates'] = options['result']
                    options['result'] = None
                    return render_template('infer.html', **options)
            else:
                raise RuntimeError('Did you click the Ask button?')
    except Exception as e:
        log(e)
        if str(e) == 'TSocket read 0 bytes':
            e = 'Back-end service encountered a problem'
        options['error'] = e
        return render_template('infer.html', **options)
    # Display.
    return render_template('infer.html', **options)

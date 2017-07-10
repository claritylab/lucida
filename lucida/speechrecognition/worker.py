import logging
import logging.handlers
import os
import click
import configuration
from gi.repository import GObject
import threading
import time

from decoder_pipeline import DecoderPipeline
from worker_socket import SocketHandler
logger = logging.getLogger(__name__)

SILENCE_TIMEOUT = 2
INITIAL_SILENCE_TIMEOUT = 5
RESPONSE_TIMEOUT = 30
CONNECT_TIMEOUT = 5
MAX_DURATION = 1800

DECODERS = []
for decoder in next(os.walk(os.path.dirname(os.path.realpath(__file__)) + "/decoders/"))[1]:
    if os.path.isfile(os.path.dirname(os.path.realpath(__file__)) + "/decoders/" + decoder + "/decoder") and os.access(os.path.dirname(os.path.realpath(__file__)) + "/decoders/" + decoder + "/decoder", os.X_OK):
        DECODERS.append(decoder)

@click.command()
@click.option("--decoder", prompt="Select speech to text decoder " + str(DECODERS), type=click.Choice(DECODERS), required=True, help="Speech to text decoder")
@click.option("--threads", prompt="Enter number of decoders to run in parallel [1-500]", default=1, type=click.IntRange(min=1, max=500), required=True, show_default=True, help="Number of decoders to run in parallel")
def main(decoder, threads):
    conf = configuration.load()

    global SILENCE_TIMEOUT
    SILENCE_TIMEOUT = conf['silence_timeout']
    global INITIAL_SILENCE_TIMEOUT
    INITIAL_SILENCE_TIMEOUT = conf['initial_silence_timeout']
    global RESPONSE_TIMEOUT
    RESPONSE_TIMEOUT = conf['response_timeout']
#    global CONNECT_TIMEOUT
#    CONNECT_TIMEOUT = conf['connect_timeout']
#    global MAX_DURATION
#    MAX_DURATION = conf['max_duration']

    logger.setLevel(conf['worker_verbosity'])
    os.environ['GST_DEBUG'] = conf['gstreamer_verbosity']

    conf['decoder'] = decoder

    if threads > 1:
        import tornado.process
        logging.info("Forking into %d processes" % threads)
        tornado.process.fork_processes(args.fork)

    pipeline = DecoderPipeline(conf)

    GObjectLoop = GObject.MainLoop()
    GObjectThread = threading.Thread(target=GObjectLoop.run)
    GObjectThread.daemon = True
    GObjectThread.start()

    while True:
        ws = SocketHandler(conf['master'], pipeline, conf)
        try:
            logger.info("Opening websocket connection to master server")
            ws.connect()
            ws.run_forever()
        except Exception:
            logger.error("Couldn't connect to server, waiting for %d seconds", CONNECT_TIMEOUT)
            time.sleep(CONNECT_TIMEOUT - 1)
        time.sleep(1)

if __name__ == "__main__":
    main()


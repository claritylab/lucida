Kaldi GStreamer server
======================
[![GitHub license](https://img.shields.io/github/license/alumae/kaldi-gstreamer-server.svg)](https://github.com/alumae/kaldi-gstreamer-server/blob/master/LICENSE)
[![Code Climate](https://img.shields.io/codeclimate/github/alumae/kaldi-gstreamer-server.svg)](https://codeclimate.com/github/alumae/kaldi-gstreamer-server)

This is a real-time full-duplex speech recognition server, based on
the Kaldi toolkit and the GStreamer framework and implemented in Python.

Notes for Lucida Users
======================

Build
--------

```
make
```

It runs `install_kaldi.sh` only if the directory `kaldi` does not exist.
Therefore, if previous intallation fails, please remove this directory and compile again.
It may take up to 4 hours to complete.

Run
--------

Start the master server followed by the worker:

```
make start_master_server
make start_server
```

Wait until you see `Opened websocket connection to server` from the worker.`

Test
--------

Test the installtion process:

```
make start_test
```

If you see results popping up, it should be ready to go.

Text to Speech
--------

In addition to the speech recognition service, we provide a text-to-speech (TTS) service based on [kaldi idlak](https://github.com/bpotard/idlak). 
Currently, it is experimental because our web front end uses the [web APIs](https://developer.mozilla.org/en-US/docs/Web/API/SpeechSynthesis) to perform voice synthesis. 
However, you are welcome to use this service by running the script `install_kaldi_tts.sh`.
It installs both the kaldi toolkit (~4 hours for C++ compilation) and the idlak component (~4 hours for DNN training), and at the end of this process, we provide one way to use it --
convert text input from command line to audio file saved on disk.

(End of Notes for Lucida Users)

Features
--------

  * Full duplex communication based on websockets: speech goes in, partial hypotheses come out (think of Android's voice typing)
  * Very scalable: the server consists of a master component and workers; one worker is needed per concurrent recognition session; workers can be
    started and stopped independently of the master on remote machines
  * Can do speech segmentation, i.e., a long speech signal is broken into shorter segments based on silences
  * Supports arbitrarily long speech input (e.g., you can stream live speech into it)
  * Supports Kaldi's GMM and "online DNN" models
  * Supports rescoring of the recognition lattice with a large language model
  * Supports persisting the acoustic model adaptation state between requests
  * Supports unlimited set of audio codecs (actually only those supported by GStreamer)
  * Supports rewriting raw recognition results using external programs (can be used for converting words to numbers, etc)
  * Python, Java, Javascript clients are available

English demo that uses the server: http://bark.phon.ioc.ee/dictate/

Estonian demo: http://bark.phon.ioc.ee/dikteeri/

Changelog
---------
  * 2015-12-04: added a link to the Dockerfile.

  * 2015-06-30: server now uses the recently added "full final results" functionality of gst-kaldi-nnet2-online.
  Full results can include things like n-best hypotheses, word and phone alignment information, 
  and possibly other things in the future. You have to upgrade gst-kaldi-nnet2-online (when using this plugin instead of
  the GMM-based Kaldi GStreamer plugin) prior to using this. Also added a sample full results post-processing
  script `sample_full_post_processor.py` (see `sample_english_nnet2.yaml` on how to use it).
  
  

Installation
------------

### Docker

Building Kaldi and all the other packages required by this software can be quite complicated. Instead of building
all the prerequisites manually, one could use the Dockerfile created by José Eduardo Silva: https://github.com/jcsilva/docker-kaldi-gstreamer-server.

### Requirements

#### Python 2.7 with the following packages:

  * Tornado 4, see http://www.tornadoweb.org/en/stable/
  * ws4py (0.3.0 .. 0.3.2)
  * YAML
  * JSON

*NB!*: The server doesn't work quite correctly with ws4py 0.3.5 because of a bug I reported here: https://github.com/Lawouach/WebSocket-for-Python/issues/152.
Use ws4py 0.3.2 instead. To install ws4py 0.3.2 using `pip`, run:

    pip install ws4py==0.3.2

In addition, you need Python 2.x bindings for gobject-introspection libraries, provided by the `python-gi`
package on Debian and Ubuntu.

#### Kaldi

Download and compile Kaldi (http://kaldi.sourceforge.net). Also compile the online extensions (`make ext`)
and the Kaldi GStreamer plugin (see `README` in Kaldi's `src/gst-plugin` directory).

#### Acoustic and language models for Kaldi

You need GMM-HMM-based acoustic and n-gram language models (actually their FST cascade) for your language.

Working (but not very accurate) recognition models are available for English and Estonian in the  `test/models/` directory.
English models are based on Voxforge acoustic models and the CMU Sphinx  2013 general English trigram language model (http://cmusphinx.sourceforge.net/2013/01/a-new-english-language-model-release/).
The language models were heavily pruned so that the resulting FST cascade would be less than the
100 MB GitHub file size limit.

*Update:* the server also supports Kaldi's new "online2" online decoder that uses DNN-based acoustic models with i-vector input. See below on
how to use it. According to experiments on two Estonian online decoding setups, the DNN-based models result in about 20% (or more) relatively less
errors than GMM-based models (e.g., WER dropped from 13% to 9%).


Running the server
------------------

### Running the master server

The following starts the main server on localhost:8888

    python kaldigstserver/master_server.py --port=8888

### Running workers


The master server doesn't perform speech recognition itself, it simply delegates client recognition
requests to workers. You need one worker per recognition session. So, the number of running workers
should be at least the number of potential concurrent recognition sessions. Good thing is that
workers are fully independent and do not even have to be running on the same machine, thus
offering practically unlimited parallelness.

There are two decoders that a worker can use: based on the Kaldi `onlinegmmdecodefaster` GStreamer plugin
or based on the newer `kaldinnet2onlinedecoder` plugin. The first one supports GMM models, the latter one needs
"online2" DNN-based models with i-vector input.

To run a worker, first write a configuration file. A sample configuration that uses the English GMM-HMM
models that come with this project is available in `sample_worker.yaml`. A sample worker that uses
"online2" DNN-based models is in `sample_english_nnet2.yaml`.

#### Using the 'onlinegmmdecodefaster' based worker

Before starting a worker, make sure that the GST plugin path includes Kaldi's `src/gst-plugin` directory
(which should contain the file `libgstkaldi.so`), something like:

    export GST_PLUGIN_PATH=~/tools/kaldi-trunk/src/gst-plugin

Test if it worked:

    gst-inspect-1.0 onlinegmmdecodefaster

The latter should print out information about the Kaldi's GStreamer plugin.

Now, you can start a worker:

    python kaldigstserver/worker.py -u ws://localhost:8888/worker/ws/speech -c sample_worker.yaml

The `-u ws://localhost:8890/worker/ws/speech` argument specifies the address of the main server
that the worker should connect to. Make sure you are using the same port as in the server invocation.

You can start any number of worker processes, just use the same command to start the next workers.

It might be a good idea to use [supervisord](http://supervisord.org) to start and stop the main server and
several workers. A sample supervisord configuration file is in `etc/english-supervisord.conf`.

Server usage
------------

A sample implementation of the client is in `kaldigstserver/client.py`.

If you started the server/worker as described above, you should be able to test the installation by invoking:

    python kaldigstserver/client.py -r 32000 test/data/english_test.raw

Expected output:

    THE. ONE TWO THREE FOUR FIVE SIX SEVEN EIGHT.

Expected output when using using the DNN-based online models based on Fisher:

    one two or three you fall five six seven eight. yeah.

The `-r 32000` in the last command tells the client to send audio to the server at 32000 bytes per second. The raw
sample audio file uses a sample rate of 16k with a 16-bit encoding which results in a byterate of 32000.

You can also send ogg audio:

    python kaldigstserver/client.py -r 4800 test/data/english_test.ogg

The rate in the last command is 4800. The bit rate of the ogg file is 37.5k, which results in a byte rate of 4800.


Using the 'kaldinnet2onlinedecoder' based worker
------------------------------------------------

The DNN-based online decoder requires a newer GStreamer plugin that is not in the Kaldi codebase and has to be compiled
seperately. It's available at https://github.com/alumae/gst-kaldi-nnet2-online. Clone it, e.g., under `~/tools/gst-kaldi-nnet2-online`.
Follow the instuctions and compile it. This should result in a file `~/tools/gst-kaldi-nnet2-online/src/libgstkaldionline2.so`.

Also, download the DNN-based models for English, trained on the TEDLIUM speech corpus and combined with a generic English language model
provided by Cantab Research. Run the `download-tedlium-nnet2.sh` under `test/models` to download the models (attention, 1.5 GB):

    ./test/models/download-tedlium-nnet2.sh

Before starting a worker, make sure that the GST plugin path includes the path where the `libgstkaldionline2.so` library you compiled earlier
resides, something like:

    export GST_PLUGIN_PATH=~/tools/gst-kaldi-nnet2-online/src

Test if it worked:

    gst-inspect-1.0 kaldinnet2onlinedecoder

The latter should print out information about the new Kaldi's GStreamer plugin.

Now, you can start a worker:

    python kaldigstserver/worker.py -u ws://localhost:8888/worker/ws/speech -c sample_english_nnet2.yaml

As the acoustic models are trained on TED data, we also test on TED data. The file `test/data/bill_gates-TED.mp3` contains about one
minute of a TED talk by Bill Gates. It's encoded as 64 kb MP3, so let's send it to the server at 64*1024/8=8192 bytes per second:

    python kaldigstserver/client.py -r 8192 test/data/bill_gates-TED.mp3

Recognized words should start appearing at the terminal. The final result should be something like:

> when i was a kid the disaster we worry about most was a nuclear war. that's why we had a bear like this down our basement filled with cans of food and water. nuclear attack came we were supposed to go downstairs hunker down and eat out of that barrel. today the greatest risk of global catastrophe. don't look like this instead it looks like this. if anything kills over ten million people in the next few decades it's most likely to be a highly infectious virus rather than a war. not missiles that microbes now part of the reason for this is that we have invested a huge amount in nuclear deterrence we've actually invested very little in a system to stop an epidemic. we're not ready for the next epidemic.

Compare that to the original transcript in `test/data/bill_gates-TED.txt`:

> When I was a kid, the disaster we worried about most was a nuclear war. That's why we had a barrel like this down in our basement, filled with cans of food and water. When the nuclear attack came, we were supposed to go downstairs, hunker down, and eat out of that barrel.
> Today the greatest risk of global catastrophe doesn't look like this. Instead, it looks like this. If anything kills over 10 million people in the next few decades, it's most likely to be a highly infectious virus rather than a war. Not missiles, but microbes. Now, part of the reason for this is that we've invested a huge amount in nuclear deterrents. But we've actually invested very little in a system to stop an epidemic. We're not ready for the next epidemic.


#### Retrieving and sending adaptation state ####

If you use the 'kaldinnet2onlinedecoder' based worker, you can retrieve the adaptation state after the decoding session
finishes, and send the previously retrieved adaptation state when starting a new session.

The 'kaldinnet2onlinedecoder' worker always sends the adaptation state encoded in a JSON container once the session ends. Client
can store it in a file. This is functionality is implemented by the `client.py`. Assuming that you started the server and a worker as in the last
example, you can do:

    python kaldigstserver/client.py -r 32000 --save-adaptation-state adaptation-state.json test/data/english_test.wav

The `adaptation-state.json` file will contain something like this:

    {"type": "string+gzip+base64", "value": "eJxlvUuPdEmSHbavXx...", "time": "2014-11-14T11:08:49"}

As you can see, the adaptation state is not human-readable, it's actually gzipped and base64-encoded text data.

To start another decoding session using the saved adaptation state, you can do something like this:

    python kaldigstserver/client.py -r 32000 --send-adaptation-state adaptation-state.json test/data/english_test.wav



Alternative usage through a HTTP API
---------------------------------------

One can also use the server through a very simple HTTP-based API. This allows to simply send audio via a PUT or POST request
to http://server:port/client/dynamic/recognize and read the JSON ouput. Note that the JSON output is differently structured
than the output of the websocket-based API. This interface is compatible to the one implemented by http://github.com/alumae/ruby-pocketsphinx-server.

The HTTP API supports chunked transfer encoding which means that server can read and decode an audio stream before it is complete.

Example:

Send audio to server:

     curl  -T test/data/english_test.wav  "http://localhost:8888/client/dynamic/recognize"

Output:

    {"status": 0, "hypotheses": [{"utterance": "one two or three you fall five six seven eight. [noise]."}], "id": "7851281f-e187-4c24-9b58-4f3a5cba3dce"}

Send audio using chunked transfer encoding at an audio byte rate; you can see from the worker logs that decoding starts already when the first chunks
have been received:

    curl -v -T test/data/english_test.raw -H "Content-Type: audio/x-raw-int; rate=16000" --header "Transfer-Encoding: chunked" --limit-rate 32000  "http://localhost:8888/client/dynamic/recognize"

Output (like before):

    {"status": 0, "hypotheses": [{"utterance": "one two or three you fall five six seven eight. yeah."}], "id": "4e4594ee-bdb2-401f-8114-41a541d89eb8"}


Websocket-based client-server protocol
----------------------

### Opening a session

To open a session, connect to the specified server websocket address (e.g. ws://localhost:8888/client/ws/speech).
The server assumes by default that incoming audio is sent using 16 kHz, mono, 16bit little-endian format. This can be overriden
using the 'content-type' request parameter. The content type has to be specified using GStreamer 1.0 caps format,
e.g. to send 44100 Hz mono 16-bit data, use: "audio/x-raw, layout=(string)interleaved, rate=(int)44100, format=(string)S16LE, channels=(int)1".
This needs to be url-encoded of course, so the actual request is something like:

    ws://localhost:8888/client/ws/speech?content-type=audio/x-raw,+layout=(string)interleaved,+rate=(int)44100,+format=(string)S16LE,+channels=(int)1

Audio can also be encoded using any codec recognized by GStreamer (assuming the needed packages are installed on the server).
GStreamer should recognize the container and codec automatically from the stream, you don't have to specify the content type.
E.g., to send audio encoded using the Speex codec in an Ogg container, use the following URL to open the session (server should
automatically recognize the codec):

    ws://localhost:8888/client/ws/speech

### Sending audio

Speech should be sent to the server in raw blocks of data, using the encoding specified when session was opened.
It is recommended that a new block is sent at least 4 times per second (less frequent blocks would increase the recognition lag).
Blocks do not have to be of equal size.

After the last block of speech data, a special 3-byte ANSI-encoded string "EOS"  ("end-of-stream") needs to be sent to the server. This tells the
server that no more speech is coming and the recognition can be finalized.

After sending "EOS", client has to keep the websocket open to receive recognition results from the server. Server
closes the connection itself when all recognition results have been sent to the client.
No more audio can be sent via the same websocket after an "EOS" has been sent. In order to process a new
audio stream, a new websocket connection has to be created by the client.

### Reading results

Server sends recognition results and other information to the client using the JSON format.
The response can contain the following fields:

  * status -- response status (integer), see codes below
  * message -- (optional) status message
  * result -- (optional) recognition result, containing the following fields:
    - hypotheses - recognized words, a list with each item containing the following:
        + transcript -- recognized words
        + confidence -- (optional) confidence of the hypothesis (float, 0..1)
    - final -- true when the hypothesis is final, i.e., doesn't change any more

The following status codes are currently in use:

  * 0 -- Success. Usually used when recognition results are sent
  * 2 -- Aborted. Recognition was aborted for some reason.
  * 1 -- No speech. Sent when the incoming audio contains a large portion of silence or non-speech.
  * 9 -- Not available. Used when all recognizer processes are currently in use and recognition cannot be performed.

Websocket is always closed by the server after sending a non-zero status update.

Examples of server responses:

    {"status": 9}
    {"status": 0, "result": {"hypotheses": [{"transcript": "see on"}], "final": false}}
    {"status": 0, "result": {"hypotheses": [{"transcript": "see on teine lause."}], "final": true}}

Server segments incoming audio on the fly. For each segment, many non-final hypotheses, followed by one final
hypothesis are sent. Non-final hypotheses are used to present partial recognition hypotheses
to the client. A sequence of non-final hypotheses is always followed by a final hypothesis for that segment.
After sending a final hypothesis for a segment,
server starts decoding the next segment, or closes the connection, if all audio sent by the client has been processed.

Client is reponsible for presenting the results to the user in a way
suitable for the application.

Client software
---------------

Javascript client is available here: http://kaljurand.github.io/dictate.js

Citing
------

If you use this software for research, you can cite the paper where this software is
described (available here: http://ebooks.iospress.nl/volumearticle/37996):

    @inproceedigs{alumae2014,
      author={Tanel Alum\"{a}e},
      title="Full-duplex Speech-to-text System for {Estonian}",
      booktitle="Baltic HLT 2014",
      year=2014,
      address="Kaunas, Lihtuania"
    }

Of course, you should also acknowledge Kaldi, which does all the hard work.

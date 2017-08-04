Python Samples for the Google Assistant gRPC API
================================================

This repository contains a reference sample for the ``google-assistant-grpc`` Python package_.

It implements the following features:

- Triggering a conversation using a key press
- Audio recording of user queries (single or multiple consecutive queries)
- Playback of the Assistant response
- Conversation state management
- Volume control

.. _package: https://pypi.python.org/pypi/google-assistant-grpc

Prerequisites
-------------

- `Python <https://www.python.org/>`_ (>= 3.4 recommended)
- A `Google API Console Project <https://console.developers.google.com>`_
- A `Google account <https://myaccount.google.com/>`_

Setup
-----

- Install Python 3

    - Ubuntu/Debian GNU/Linux::

        sudo apt-get update
        sudo apt-get install python3 python3-venv

    - `MacOSX, Windows, Other <https://www.python.org/downloads/>`_

- Create a new virtual environment (recommended)::

    python3 -m venv env
    env/bin/python -m pip install --upgrade pip setuptools
    source env/bin/activate

Authorization
-------------

- Follow `the steps to configure the project and the Google account <https://developers.google.com/assistant/sdk/prototype/getting-started-other-platforms/config-dev-project-and-account>`_.


- Download the ``client_secret_XXXXX.json`` file from the `Google API Console Project credentials section <https://console.developers.google.com/apis/credentials>`_ and generate credentials using ``google-oauth-tool``::

    pip install --upgrade google-auth-oauthlib[tool]
    google-oauthlib-tool --client-secrets path/to/client_secret_XXXXX.json --scope https://www.googleapis.com/auth/assistant-sdk-prototype --save --headless

Run the sample
--------------

- Install the sample dependencies::

    sudo apt-get install portaudio19-dev libffi-dev libssl-dev
    pip install --upgrade -r requirements.txt

-  Verify audio setup::

    # Record a 5 sec sample and play it back
    python -m audio_helpers

- Run the push to talk sample. The sample records a voice query after a key press and plays back the Google Assistant's answer::

    python -m pushtotalk

- Send a pre-recorded request to the Assistant::

    python -m pushtotalk -i in.wav

- Save the Assistant response to a file::

    python -m pushtotalk -o out.wav

Troubleshooting
---------------

- Verify ALSA setup::

    # Play a test sound
    speaker-test -t wav

    # Record and play back some audio using ALSA command-line tools
    arecord --format=S16_LE --duration=5 --rate=16k --file-type=raw out.raw
    aplay --format=S16_LE --rate=16k --file-type=raw out.raw

- Run the sample with verbose logging enabled::

    python -m pushtotalk --verbose

- If Assistant audio is choppy, try adjusting the sound device's block size::

    # If using a USB speaker or dedicated soundcard, set block size to "0"
    # to automatically adjust the buffer size
    python -m audio_helpers --audio-block-size=0

    # If using the line-out 3.5mm audio jack on the device, set block size
    # to a value larger than the `ConverseResponse` audio payload size
    python -m audio_helpers --audio-block-size=3200

    # Run the Assistant sample using the best block size value found above
    python -m pushtotalk --audio-block-size=value

- If Assistant audio is truncated, try adjusting the sound device's flush size::

    # Set flush size to a value larger than the audio block size. You can
    # run the sample using the --audio-flush-size flag as well.
    python -m audio_helpers --audio-block-size=3200 --audio-flush-size=6400

License
-------

Copyright (C) 2017 Google Inc.

Licensed to the Apache Software Foundation (ASF) under one or more contributor
license agreements.  See the NOTICE file distributed with this work for
additional information regarding copyright ownership.  The ASF licenses this
file to you under the Apache License, Version 2.0 (the "License"); you may not
use this file except in compliance with the License.  You may obtain a copy of
the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations under
the License.

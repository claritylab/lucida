Google Assistant microservice for Lucida
========================================

NOTE: By default this microservice is for personal use only. For commercial usage contact Google for a commercial client secret.

This microservice allows Lucida to access user's Google Assistant. This microservice takes only speech input and returns speech as well as text output. Text input is handled by question answering Google Assistant library which can be found at questionanswering/googleassist.

Installation
------------

- Follow `the steps to configure the project and the Google account <https://developers.google.com/assistant/sdk/prototype/getting-started-other-platforms/config-dev-project-and-account>`_.
- Download the ``client_secret_XXXXX.json`` file from the `Google API Console Project credentials section <https://console.developers.google.com/apis/credentials>`_.
- Change directory to `speechrecognition/decoders/googleassist` and type `make`. Or simply type make all in `speechrecognition` to build all decoders and speechrecognition toolkit

Add user
-----------

This microservice uses Google Assistant and requires user's consent for the same. There are two methods to provide consent:

- Ask a question (e.g. What is Taj Mahal?) using any valid text interface and follow the instructions.
- Visit the settings section on Lucida's web interface (under development)

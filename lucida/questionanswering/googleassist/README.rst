Google Assistant microservice for Lucida
========================================

NOTE: By default this microservice is for personal use only. For commercial usage contact Google for a commercial client secret.

This microservice allows Lucida to access user's Google Assistant. This microservice takes only text input and returns text output as against the Google Assistant which accepts only voice input. Voice input is handled by speech recognition Google Assistant library which can be found at speechrecognition/decoders/googleassist.

Installation
------------

- Follow `the steps to configure the project and the Google account <https://developers.google.com/assistant/sdk/prototype/getting-started-other-platforms/config-dev-project-and-account>`_.
- Download the ``client_secret_XXXXX.json`` file from the `Google API Console Project credentials section <https://console.developers.google.com/apis/credentials>`_.
- Change directory to `questionanswering/googleassist` and type `make all` followed by `make setup`

Start microservice
------------------

- Change directory to `questionanswering/googleassist` and type `make start_server`

Add user
-----------

This microservice uses Google Assistant and requires user's consent for the same. There are two methods to provide consent::
    - Ask a question (e.g. What is Taj Mahal?) using any valid text interface and follow the instructions.
    - Visit the settings section on Lucida's web interface (under development)

License
-------

Copyright (C) 2017 Google Inc.
Copyright (C) 2017 Kamal Galrani

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

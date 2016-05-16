#!/usr/bin/env python

#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

import glob
import logging
import sys

sys.path.append('lucidaservice')
sys.path.append('lucidatypes')
sys.path.insert(0, glob.glob('/home/yba/Documents/clarity/thrift-0.9.3/lib/py/build/lib*')[0])

from lucidatypes.ttypes import *
from lucidaservice import *

from thrift import TTornado
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from tornado import gen
from tornado import ioloop


@gen.coroutine
def communicate():
    # create client
    transport = TTornado.TTornadoStreamTransport('localhost', 8084)
    # open the transport, bail on error
    try:
        yield transport.open()
        print('Transport is opened')
    except TTransport.TTransportException as ex:
        logging.error(ex)
        raise gen.Return()

    pfactory = TBinaryProtocol.TBinaryProtocolFactory()
    client = LucidaService.Client(transport, pfactory)

    # ping
    LUCID = "yba2"
    label = "island"
    data = "xxxxx"
    knowledge_input = QueryInput()
    knowledge_input.tags = []
    knowledge_input.tags.append(label)
    knowledge_input.data = []
    knowledge_input.data.append(data)
    knowledge = QuerySpec()
    knowledge.content = []
    knowledge.content.append(knowledge_input)
    yield client.learn(LUCID, knowledge_input)
    print("Learn")

    # close the transport
    client._transport.close()
    raise gen.Return()


def main():
    # create an ioloop, do the above, then stop
    ioloop.IOLoop.current().run_sync(communicate)


if __name__ == "__main__":
    main()
    
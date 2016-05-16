
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
import sys

sys.path.append('lucidaservice')
sys.path.append('lucidatypes')
sys.path.insert(0, glob.glob('/home/yba/Documents/clarity/thrift-0.9.3/lib/py/build/lib*')[0])

from lucidatypes.ttypes import *
from lucidaservice import *

from thrift import TTornado
from thrift.protocol import TBinaryProtocol

from tornado import ioloop


class LucidaServiceHandler(LucidaService.Iface):
 
    def __init__(self):
        self.log = {}
 
    def create(self, LUCID, spec):
        log('Create ' + spec.content[0].type + ' at host ' + spec.content[0].data[0] + ' port ' + spec.content[0].tags[0])
        ThriftClient.add_service(spec.content[0].type, spec.content[0].data[0], spec.content[0].tags[0])
        return
 
    def learn(self, LUCID, knowledge):
        log('####################################')
        return      
     
    def infer(self, LUCID, query): 
        return 'CMD'


def main():
    handler = LucidaServiceHandler()
    processor = LucidaService.Processor(handler)
    pfactory = TBinaryProtocol.TBinaryProtocolFactory()
    server = TTornado.TTornadoServer(processor, pfactory)

    print("Starting the server...")
    server.bind(8084)
    server.start(1)
    ioloop.IOLoop.instance().start()
    print("done.")


if __name__ == "__main__":
    main()
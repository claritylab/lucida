# Question Answer

The question answering service uses OpenEphyra, an open source project from
Carnegie Mellon.

Dependencies: You'll need to install thrift and modify `config.inc` from the top
directory to include the path to your thrift installation. Then visit
(thrift)/lib/java and run `ant`, where (thrift) is the directory where you
installed thrift.  For more information, take a look at
[this](https://thrift.apache.org/lib/java).  This version has been tested with
thrift-0.9.2.

OpenEphyra/src/info/ -- Contains the actual OpenEphyra package, with some modifications.
 You shouldn't need to modify this directory.

OpenEphyra/src/lucida/ -- Contains an OpenEphyra wrapper to communicates with Lucida, and a testing client.

## Building and starting the QA service

Navigate to `OpenEphyra`, `./compile-OpenEphyra.sh`.

To start the server, `./start-OpenEphyra-server.sh`.

To start the testing client, `./start-OpenEphyra-tester.sh`.

To change the Database used from the testing client, modify `OpenEphyra/src/lucida/test/QAClient.java`, re-compile, and re-start.

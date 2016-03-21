# Question Answer

The question answering service uses OpenEphyra, an open source project from
Carnegie Mellon.

Dependencies: You'll need to install thrift and modify `config.inc` from the top
directory to include the path to your thrift installation. Then visit
(thrift)/lib/java and run `ant`, where (thrift) is the directory where you
installed thrift.  For more information, take a look at
[this](https://thrift.apache.org/lib/java).  This version has been tested with
thrift-0.9.2.

common/ -- Contains the actual OpenEphyra package, with some modifications.
 You shouldn't need to modify this directory.

lucida/ -- Contains an OpenEphyra wrapper to communicates with Lucida.

template/ -- Contains a stand-alone OpenEphyra wrapper for use in any application.

## Building and starting the QA service
Navigate to `lucida` or `template` and follow the README
To change the Database used, edit `common/qa-runtime-config.inc`

Last Modified: 07/01/15

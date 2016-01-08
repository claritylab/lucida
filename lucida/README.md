# Lucida
Lucida is an ecosystem of micro-services that make RPC calls using [Apache
Thrift](http://thrift.apache.org/). Each service is built using common Thrift
interface making micro-service communication efficient and flexible. The command
center arbitrates communication between the services. Lucida has been tested with Ubuntu 14.04.

Current micro-services:
- Automatic Speech Recognition (asr)
- Question-Answering (qa)
- Image Matching (imm)
- DjiNN and Tonic (coming soon)

## Dependencies
Lucida depends on the following packages and their dependencies (many of which can be installed from the package manager):
- [Boost 1.54.0](http://www.boost.org/)
- [Thrift 0.9.2](https://thrift.apache.org/)
- [Kaldi](http://kaldi-asr.org/): A snapshot is provided but the dependencies still need to be installed.
- [Libsox](http://sox.sourceforge.net/)
- [OpenEphyra](https://mu.lti.cs.cmu.edu/trac/Ephyra/wiki/OpenEphyra): A snapshot is provided but the dependencies still need to be installed.
- [OpenCV 2.4.9](http://opencv.org/)
- [Tesseract OCR](https://github.com/tesseract-ocr/tesseract)
- [Protobuf 2.5.0](https://developers.google.com/protocol-buffers/?hl=en)

## Building Lucida
- Download and build the C++ and Java bindings for [Apache
  Thrift](http://thrift.apache.org/)
- Build the [command center](command-center)
- Build each micro-service following each provided README

## Learn
Use the learning service to teach Lucida new knowledge from a range of different sources. Check out the provided [Readme](learn).

## Queries
- Start the command center first following the instructions in the command
  center's README
- Start each of the services separately assuring they correctly register with
  the command center (check the output)
- A command center client is provided in [command center](command-center) to
  test the command center and the micro services' execution.

# Lucida
Lucida is an ecosystem of micro-services that make RPC calls using [Apache
Thrift](http://thrift.apache.org/). Each service is built using common Thrift
interface making micro-service communication efficient and flexible. The command
center arbitrates communication between the services.

Current micro-services:
- Automatic Speech Recognition (asr)
- Question-Answering (qa)
- Image Matching (imm)
- DjiNN and Tonic (coming soon)

## Building Lucida
- Download and build the C++ and Java bindings for [Apache
  Thrift](http://thrift.apache.org/)
- Build the [command center](command-center)
- Build each micro-service following each provided README

## Queries
- Start the command center first following the instructions in the command
  center's README
- Start each of the services separately assuring they correctly register with
  the command center (check the output)
- A command center client is provided in [command center](command-center) to
  test the command center and the micro services' execution.

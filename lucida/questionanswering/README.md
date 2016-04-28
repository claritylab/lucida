# Question Answering (QA)

The current implementation of QA uses OpenEphyra, an open source project from
Carnegie Mellon.

## Notes:

1. `OpenEphyra` contains the implementation of the OpenEphyra QA service.

2. If you want to create and use another QA implementation,
you can start by making a directory parallel to `OpenEphyra` and modify `Makefile`.
Make sure to reference `../lucidaservice.thrift` and `../lucidatypes.thrift`.

3. Type `make` to build all QA implementations,
or type `cd OpenEphyra` and `make` to only build the OpenEphyra QA service.

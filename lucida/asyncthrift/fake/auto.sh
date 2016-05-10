#!/bin/bash
cd fakecmd_py
make
make start_server 
cd ..
cd fakeqa
make
make start_server
cd ..
cd fakeimm
make
make start_server
cd ..
cd fakecmd_py
make start_client

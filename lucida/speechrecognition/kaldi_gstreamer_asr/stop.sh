#!/bin/bash

#kill worker
ps axf | grep worker.py | grep -v grep | awk '{print "kill -15 " $1}' | sh

#kill master
ps axf | grep master_server.py | grep -v grep | awk '{print "kill -15 " $1}' | sh


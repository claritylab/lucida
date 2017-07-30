#!/bin/bash

thrift -o ../include --gen c_glib asrthriftservice.thrift
thrift -o ../include --gen py asrthriftservice.thrift

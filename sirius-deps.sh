#!/bin/bash/env bash

# Install (some) dependencies

if [ "$EUID" -ne 0 ]
then
  echo >&2 "$0: [ERROR] Sudo access required. Aborting."
  exit 1
fi

apt-get install \
  default-jdk ant automake autoconf libtool bison libboost-all-dev

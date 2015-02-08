#!/usr/bin/env bash

if [ "$EUID" -ne 0 ]
then
  echo >&2 "$0: [ERROR] Sudo access required. Aborting."
  exit 1
fi

git clone https://github.com/chokkan/liblbfgs.git
cd liblbfgs/ ;
./autogen.sh && ./configure
make && make install

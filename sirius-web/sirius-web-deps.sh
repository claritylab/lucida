#!/usr/bin/env bash

if [ "$EUID" -ne 0 ]
then
  echo >&2 "$0: [ERROR] Sudo access required. Aborting."
  exit 1
fi

apt-get install python-pip python-pyaudio
pip install wtforms Flask requests pickledb

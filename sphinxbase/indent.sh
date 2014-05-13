#!/bin/sh

# This is the equivalent of the following Emacs thing:
# -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

find . -name '*.c' -print0 | xargs -0 indent -i4 -kr -psl -nce -nut

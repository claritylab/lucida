#!/bin/sh

FEATURE="mfcc"
#FEATURE="gt"
#FEATURE="plp"
#FEATURE="mrasta"

./executables/feature-extraction.* \
    --config=config/feature-extraction.${FEATURE}.config

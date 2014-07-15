#!/bin/sh
# extract features; the default setup only makes use of MFCCs;
# for other features you need to modify config files;

FEATURE="mfcc"
#FEATURE="gt"
#FEATURE="plp"
#FEATURE="mrasta"

./executables/feature-extraction.* \
    --config=config/feature-extraction.$FEATURE.config

#!/bin/sh
set -e

./executables/feature-statistics.* \
    --config=config/total-scatter-estimation.config

./executables/feature-statistics.* \
    --config=config/total-scatter-normalization.config

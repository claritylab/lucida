#!/bin/sh

AMT=./executables/acoustic-model-trainer.*

$AMT --config=config/monophone-alignment.config

$AMT --config=config/cart-accumulation.config

$AMT --config=config/cart-estimation.config --ITER=1

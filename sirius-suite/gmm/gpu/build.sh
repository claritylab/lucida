#!/bin/bash

nvcc -arch=sm_35 gmm_scoring.cu -o gmm_scoring

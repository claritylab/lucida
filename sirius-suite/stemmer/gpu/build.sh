#!/bin/bash

nvcc -arch=sm_35 porter.cu -o porter_stem

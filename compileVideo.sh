#!/bin/bash

CPLUS_INCLUDE_PATH=/usr/include/opencv4
export CPLUS_INCLUDE_PATH

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
export LD_LIBRARY_PATH

g++ Source/Source.cpp -o Video -O3 -I /usr/local/include -L /usr/local/lib -lopencv_core -lopencv_highgui -lopencv_videoio -fopenmp -lm

#!/bin/sh

mkdir tempOpenCV && cd tempOpenCV

wget -O opencv.zip https://github.com/opencv/opencv/archive/master.zip
unzip opencv.zip

mkdir -p build && cd build

cmake  ../opencv-master
make

sudo make install

cd ..
cd ..

rm -rf tempOpenCV

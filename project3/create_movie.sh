#!/bin/bash

# automatically run mandelmovie and stitch pictures together to create movie

if [ ! -d "./pics" ]; then
    echo "creating pics directory..."
    mkdir ./pics
fi
echo "compiling..."
make > /dev/null
echo "creating pictures..."
./mandelmovie > /dev/null
echo "creating movie..."
ffmpeg -hide_banner -loglevel panic -y -i "./pics/mandel%d.bmp" mandel.mpg > /dev/null
echo "cleaning up..."
make clean > /dev/null
exit 0

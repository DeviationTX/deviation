#!/bin/bash

mkdir deviation-build
cd deviation-build
curl -o fltk-1.3.0-source.tar.gz http://ftp.easysw.com/pub/fltk/1.3.0/fltk-1.3.0-source.tar.gz
tar xzvf fltk-1.3.0-source.tar.gz
cd fltk-1.3.0
./configure
make
sudo make install
cd ..

curl -o pa_stable_v19_20111121.tgz http://www.portaudio.com/archives/pa_stable_v19_20111121.tgz
tar xzvf pa_stable_v19_20111121.tgz
cd portaudio
./configure
make
cd ..

hg clone https://bitbucket.org/PhracturedBlue/deviation
cd deviation/src
make TARGET=emu_devo8 LFLAGS="`fltk-config --ldflags`"


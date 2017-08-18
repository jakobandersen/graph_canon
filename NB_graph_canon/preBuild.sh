#!/bin/bash
cd ..
./bootstrap.sh && autoreconf -fi
mkdir -p build
cd build
export CXXFLAGS=-ftemplate-backtrace-limit=0
../configure \
	--prefix=$(pwd)/stage \
	--with-boost=$HOME/programs \
	--with-perm_group=$HOME/stuff/code/perm_group

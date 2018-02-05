#!/bin/bash
cd ../build
make -j 7 && make install "$@"

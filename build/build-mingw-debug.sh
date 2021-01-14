#!/bin/sh
cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug ../src
make

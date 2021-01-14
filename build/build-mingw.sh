#!/bin/sh
cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release ../src
make

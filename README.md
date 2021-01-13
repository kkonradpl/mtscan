MTscan
=======

MikroTik RouterOS wireless scanner.

![Screenshot](/mtscan.png?raw=true)

Copyright (C) 2015-2021  Konrad Kosmatka

https://fmdx.pl/mtscan/

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

# Build
In order to build MTscan you will need:
- CMake
- C compiler

You will also need several dependencies:
- GTK+ 2 & dependencies
- libssh (-lssh)
- yajl (-lyajl)
- zlib (-lz)
- openssl (-lcrypto)
- libcurl (-lcurl)

Once you have all the necessary dependencies, you can use scripts available in the `build` directory.

# Installation
After a successful build, just use:
```sh
$ sudo make install
```
in the `build` directory.

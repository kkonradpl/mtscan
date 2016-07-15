#!/bin/bash
cp -r 16x16/ 22x22/ 24x24/ 32x32/ 48x48/ scalable/ /usr/share/icons/hicolor/
gtk-update-icon-cache /usr/share/icons/hicolor/

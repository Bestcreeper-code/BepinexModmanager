#!/bin/bash

make debug -j4
if [ $? -ne 0 ]; then
    printf "\e[31mBuild failed\e[0m\n"
    exit 1
fi
wine ./manager.exe --bepinex-scan-file bepinexdlls_test.tmp
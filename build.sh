#!/bin/bash
set -o xtrace

mkdir -p out
cd base
make
cp out/firmware.bin ../out/firmware-base.bin
cd ..
cd remote
make
cp out/firmware.bin ../out/firmware-remote.bin

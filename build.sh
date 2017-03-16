#!/bin/bash
set -eu -o xtrace

make -C base release
make -C remote release

mkdir -p out
cp base/out/firmware.bin out/firmware-base.bin
cp remote/out/firmware.bin out/firmware-remote.bin

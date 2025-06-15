#!/bin/bash

set -eu

cd t4co/src
bash build.sh
cd ../../pico/src
bash build.sh
cd ../../gui/src
bash build.sh
cd ../..

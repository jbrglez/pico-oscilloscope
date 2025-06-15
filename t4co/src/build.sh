#!/bin/bash

set -eu

CFLAGS="-Wall -Wextra -ggdb"

mkdir -p ../build

gcc $CFLAGS -o ../build/t4co-elf2uf2 ../src/t4co-elf2uf2.c
gcc $CFLAGS -o ../build/t4co-padchecksum ../src/t4co-padchecksum.c
gcc $CFLAGS -o ../build/t4co-pasm ../src/pio_asm.c

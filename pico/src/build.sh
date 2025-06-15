#!/bin/bash

set -eu

CC="arm-none-eabi-gcc"
LD="arm-none-eabi-ld"
OBJCOPY="arm-none-eabi-objcopy"

CFLAGS="-Wall -Wno-unused-function -Wno-unused-variable -O2 -ffreestanding -nostartfiles -mthumb -mcpu=cortex-m0plus"
LFLAGS="-nostdlib"

# CFLAGS+=" -g"
# CFLAGS+=" -DDBG_LOG"
# CFLAGS+=" -DDBG_USB_EP4_XFER"

BASE='..'
SRC="${BASE}/src"
BLD="${BASE}/build"
TMP="${BLD}/tmp"
GEN="${SRC}/generated"

TOOL="${BASE}/../t4co/build"

mkdir -p $BLD
mkdir -p $TMP
mkdir -p $GEN


$TOOL/t4co-pasm $GEN/pio_programs.h $SRC/pio_sig_gen.pio

$CC $CFLAGS -c -o $BLD/boot2.o -fpic $SRC/boot2.S
$CC $CFLAGS -c -o $BLD/crt0.o  -fpic $SRC/crt0.S
$CC $CFLAGS -c -o $BLD/pico_main.o   $SRC/pico_main.c

$LD $LFLAGS -T $SRC/link.ld -o $BLD/oscilloscope.elf $BLD/boot2.o $BLD/crt0.o $BLD/pico_main.o

# Calculate and put the CRC32 sum at the end of boot2 section.
$OBJCOPY -O binary --only-section=.boot2 $BLD/oscilloscope.elf $TMP/boot2.bin
$TOOL/t4co-padchecksum $TMP/boot2.bin $TMP/crc32_boot2.bin
$OBJCOPY --update-section .boot2=$TMP/crc32_boot2.bin $BLD/oscilloscope.elf $BLD/oscilloscope.elf

# Make .uf2 file, for USB-flashing.
$OBJCOPY -O binary $BLD/oscilloscope.elf $TMP/oscilloscope.bin
$TOOL/t4co-elf2uf2 $TMP/oscilloscope.bin $BLD/oscilloscope.uf2

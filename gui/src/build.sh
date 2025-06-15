#!/bin/bash

set -eu


check_file_exists() {
	local dir_name="$2"

	if [[ -d "${!dir_name:=raylib}" ]]; then
		if [[ -e "${!dir_name}/$1" ]]; then
			: # echo "Found ${!dir_name}/$1"
		else
			echo "Could not find file $1 in ./${!dir_name}"
			exit
		fi
	elif [[ -e "${!dir_name}" ]]; then
		echo "./${!dir_name} is not a directory."
		exit
	elif [[ "${!dir_name}" != "raylib" ]]; then
		echo "./${!dir_name} does not exist."
		exit
	else
		echo "Could not find file $1"
		exit
	fi
}


# RAYLIB_INCLUDE_DIR="path/to/raylib/include"     # raylib.h, raymath.h, rlgl.h
# RAYLIB_LIB_DIR="path/to/raylib/lib"             # libraylib.a


check_file_exists "raylib.h"  RAYLIB_INCLUDE_DIR
check_file_exists "raymath.h" RAYLIB_INCLUDE_DIR
check_file_exists "rlgl.h"    RAYLIB_INCLUDE_DIR
check_file_exists "libraylib.a" RAYLIB_LIB_DIR


CFLAGS=" -Wall -Wextra -Wno-unused-function -march=native -O2 "
LFLAGS=" -lm -lusb-1.0 -L$RAYLIB_LIB_DIR -lraylib "

# CFLAGS+=" -g"
# CFLAGS+=" -DDEBUG_MODE"

mkdir -p ../build

gcc $CFLAGS -I$RAYLIB_INCLUDE_DIR -o ../build/oscilloscope ../src/oscilloscope.c  $LFLAGS

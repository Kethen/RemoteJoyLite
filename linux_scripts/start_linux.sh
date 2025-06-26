#!/bin/bash
set -xe
script_dir=$(dirname "$0")
script_dir=$(realpath "$script_dir")

cd $script_dir;

export WINEPREFIX="$(pwd)/wine_prefix"
export WINEDLLOVERRIDES=d3d9=n,b

MODE="JA"

if [ -n "$(echo $0 | grep EN)" ]
then
	MODE="EN"
fi

EXE=RemoteJoyLite_${MODE}_32.exe

if ! [ -e "$EXE" ]
then
	EXE=RemoteJoyLite_${MODE}_64.exe
fi

if ! [ -e "$EXE" ]
then
	EXE=RemoteJoyLite_${MODE}_aarch64.exe
fi

wine "$EXE"

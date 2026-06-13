#!/bin/bash

# external-updater provides:
# $1 Path to the new version.
# $2 Path to the old version.

if [ "$#" -lt 1 ]; then
  echo 'need at least the path to the current version!'
  exit 1
fi
cd $1

set -e

rm -rf .config generated/ android/

function generate() {
  which=$1
  echo -e "\n-------- $1\n"

  # These are the only generated files we actually need.
  files="config.h flags.h globals.h help.h newtoys.h tags.h"

  cp config-$which .config
  NOBUILD=1 scripts/make.sh
  out=android/$which/generated/
  mkdir -p $out
  for f in $files; do cp generated/$f $out/$f ; done
  rm -rf .config generated/

  make allnoconfig KCONFIG_ALLCONFIG=config-$which
}

generate "device"
generate "linux"
generate "mac"

rm -rf .config

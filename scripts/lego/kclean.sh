#!/bin/sh

clean_up() {
  rm -vf $filelog
  find "$(dirname $filelog)" -name *.lego.updated | xargs rm -vf
}

trap clean_up EXIT

filelog="$1"
[ -f $filelog ] || exit 0

sort -ru < $filelog | while IFS= read -r line
do
  # Current format example
  # {module}:{method}:{path}:{kernel(optional)}
  module=$(echo $line | cut -d':' -f 1)
  method=$(echo $line | cut -d':' -f 2)
  path=$(echo $line | cut -d':' -f 3)
  if [ "$method" = "C" ]
  then
    rm -vf "$path"
  elif [ "$method" = "U" ]
  then
    if [ "$module" = "NONE" ]
    then
      # Update Kconfig for source or Update Makefile for obj-y
      add_lines=$(grep "ADDED BY LEGO" $path || true)
      # clean up
      [ -f "${path}" ] && [ -n "$add_lines" ] && echo "CLEAR     ${path}" \
        && cat ${path} | grep -v "ADDED BY LEGO" > ${path}.tmp && mv ${path}.tmp ${path}
    else
      bpath="$(dirname $path)/.$(basename $path).lego.updated"
      [ -f "$bpath" ] && echo "ROLLBACK  $path" && mv -vf "$bpath" "$path"
    fi
  fi
done

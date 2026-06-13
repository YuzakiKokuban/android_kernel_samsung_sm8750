#!/bin/bash

#
# Setup.
#

# Copy the toybox tests across.
if [[ $(adb shell getprop ro.debuggable) == 1 ]]; then
  adb shell su root rm -rf /data/local/tmp/toybox-tests/
fi
adb shell rm -rf /data/local/tmp/toybox-tests/
adb shell mkdir /data/local/tmp/toybox-tests/
adb push tests/ /data/local/tmp/toybox-tests/
adb push scripts/runtest.sh /data/local/tmp/toybox-tests/

# Make a temporary directory on the device.
tmp_dir=`adb shell mktemp --directory /data/local/tmp/toybox-tests-tmp.XXXXXXXXXX`

if tty -s; then
  green="\033[1;32m"
  red="\033[1;31m"
  plain="\033[0m"
else
  green=""
  red=""
  plain=""
fi

# Force pty allocation (http://b/142798587).
dash_t="-tt"

test_toy() {
  toy=$1

  echo

  location=$(adb shell "which $toy")
  if [ -z "$location" ]; then
    echo "-- $toy not present"
    return
  fi

  echo "-- $toy"

  implementation=$(adb shell "realpath $location")
  non_toy=false
  if [ "$implementation" != "/system/bin/toybox" ]; then
    echo "-- note: $toy is *not* toybox; this does not count as a test failure"
    non_toy=true
  fi

  adb shell $dash_t "\
      export C=\"\$(which $toy)\"; \
      export CMDNAME=$toy; \
      export TESTDIR=$tmp_dir; \
      export FILES=/data/local/tmp/toybox-tests/tests/files/ ; \
      export LANG=en_US.UTF-8; \
      export VERBOSE=1 ; \
      mkdir $tmp_dir/$toy && cd $tmp_dir/$toy ; \
      source /data/local/tmp/toybox-tests/runtest.sh ; \
      source /data/local/tmp/toybox-tests/tests/$toy.test ; \
      if [ "\$FAILCOUNT" -ne 0 ]; then exit 1; fi; \
      cd .. && rm -rf $toy"
  if [ $? -eq 0 ]; then
    pass_count=$(($pass_count+1))
  elif [ "$non_toy" = "true" ]; then
    non_toy_failures="$non_toy_failures $toy"
  else
    if [[ "$toy" = "vi" ]]; then
      non_toy_failures="$non_toy_failures $toy"
    else
      failures="$failures $toy"
    fi
  fi
}

#
# Run the selected test or all tests.
#

failures=""
pass_count=0
if [ "$#" -eq 0 ]; then
  # Run all the tests.
  for t in tests/*.test; do
    toy=`echo $t | sed 's|tests/||' | sed 's|\.test||'`
    test_toy $toy
  done
else
  # Just run the tests for the given toys.
  for toy in "$@"; do
    test_toy $toy
  done
fi

#
# Show a summary and return a meaningful exit status.
#

echo
echo "_________________________________________________________________________"
echo
echo -e "${green}PASSED${plain}: $pass_count"
for failure in $failures; do
  echo -e "${red}FAILED${plain}: $failure"
done
for failure in $non_toy_failures; do
  echo -e "${red}FAILED${plain}: $failure (ignoring)"
done

# We should have run *something*...
if [ $pass_count -eq 0 ]; then exit 1; fi
# And all failures are bad...
if [ -n "$failures" ]; then exit 1; fi
exit 0

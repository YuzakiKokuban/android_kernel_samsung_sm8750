#!/bin/sh

#Copyright (c) 2020, 2021 The Linux Foundation. All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#
#1. Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
#2. Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
#3. Neither the name of the copyright holder nor the names of its contributors
#   may be used to endorse or promote products derived from this software
#   without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#MPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#THE POSSIBILITY OF SUCH DAMAGE.
#
#SPDX-License-Identifier: BSD-3-Clause

number=1
res=1

if [ ! "$1" ]
then
	echo "usage: gbmtest_all <Max Test cases to execute>"
	echo "Default Max Tests are 25"
	MAX_TEST_CASE=25
else
	MAX_TEST_CASE=$1
fi
CHECK_TEST_CASE=`expr $MAX_TEST_CASE + 1`
killall weston

while [ "$number" -lt "$CHECK_TEST_CASE" ]
do
  /usr/bin/gbmtest "$number" > /dev/null 2>&1
  res=$?
  echo "TEST " | tr -d '\n';
  if [ "$res" == 0 ] #success if 0 fail if 1
  then
	test_pass="$test_pass $number"
	echo $number | tr -d '\n'; echo " - PASSED"
  else
	test_fail="$test_fail $number"
	echo $number | tr -d '\n'; echo " - FAILED"
  fi
  number=`expr $number + 1`
done

echo ""; echo "RESULTS:"

echo "Passing Test Cases:"; echo ""

#print passing test case numbers
echo ${test_pass} ; echo ""

echo "Failing Test Cases:"

echo ""

#print failing test case numbers
echo "${test_fail}"; echo ""

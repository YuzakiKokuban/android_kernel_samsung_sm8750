#!/bin/bash
#==========================================================================
#Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are
#met:
#* Redistributions of source code must retain the above copyright
#notice, this list of conditions and the following disclaimer.
#* Redistributions in binary form must reproduce the above
#copyright notice, this list of conditions and the following
#disclaimer in the documentation and/or other materials provided
#with the distribution.
#* Neither the name of The Linux Foundation nor the names of its
#contributors may be used to endorse or promote products derived
#from this software without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
#WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
#ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
#BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
#BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
#IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================

#==========================================================================
#
# Usage:
#     mkintegrationapidoc.sh [output-path]
#
# Note, this script requires the existence of the doxygen tool -
# This script can be called from any directory
#==========================================================================

DIR="$( cd "$( dirname "$0" )" && pwd )"
# the default doxygen configuration is LocationIntegrationApiDoxygen.conf
CONFIG=$DIR/LocationIntegrationApiDoxygen.conf
OUT="docs"

# Show help message if requested, otherwise create output folder
if [ -n "$1" ]
then
    if [ "$1" == "-h" ] || [ "$1" == "--help" ]
    then
        echo "$0 [output-path]"
        echo "  e.g. $0"
        echo "       $0 docs"
        exit
    else
        OUT=$1
        if [ ! -d $OUT ]
        then
            mkdir -p $OUT
        fi
    fi
fi


which doxygen
if [ "$?" == 0 ]
then
# Use ? here to seperate patterns as / in path will be regarded as splitter by default
    sed -i "s?^OUTPUT_DIRECTORY       .*?OUTPUT_DIRECTORY       = $OUT?" $CONFIG
    sed -i "s?^INPUT    .*?INPUT                  = $DIR/inc?" $CONFIG
    doxygen $CONFIG > /dev/null 2>&1
else
    echo "This script requires doxygen tool be to installed. "
    echo "You can install is with e.g. sudo apt-get install doxygen"
    exit 1
fi


if [ ! -e $OUT/html/index.html ]
then
    echo "Error building Location Client Api doc files."
    exit 2
fi

echo
echo "doxygen docs for Location Client Api available at: $OUT/html"

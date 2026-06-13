#!/bin/bash
##################################################################################################
## Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are
## met:
##     * Redistributions of source code must retain the above copyright
##       notice, this list of conditions and the following disclaimer.
##     * Redistributions in binary form must reproduce the above
##       copyright notice, this list of conditions and the following
##       disclaimer in the documentation and/or other materials provided
##       with the distribution.
##     * Neither the name of The Linux Foundation nor the names of its
##       contributors may be used to endorse or promote products derived
##       from this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
## WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
## MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
## ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
## BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
## BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
## WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
## OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
## IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##################################################################################################

##################################################################################################
##
## location_api_msg_protobuf_gen.sh
## Description:
## Util script to compile the proto files using protoc to generate the header and C file.
## This is intended to be run once you have made a full build and if any changes are made to proto
## files or if any new proto files are added. This relieves the burden of developer in finding the
## path to protobuf work folder and running those commands.
##
## This script is also used in pre compile stage to compile the proto files so that the generated
## C and header file are based on the protoc compiler for that workspace (LV or LE).
##
## For more details, pls refer the below link
##          - https://developers.google.com/protocol-buffers/docs/reference/cpp-generated
##
## Note:
## 1. Add any new .proto files to "src_proto_files" array
##
##################################################################################################

echo;
echo "location_api_msg_protobuf_gen.sh"
echo "********************************"
# find workspace path
wsp_path=`repo info platform/hardware/qcom/gps 2>&1|grep -o "Mount path:.*"|sed 's/Mount path: //'|sed 's/hardware\/qcom\/gps//'`
echo "Workspace path is: $wsp_path"
# find manifest
manifest_info=`repo info . 2>/dev/null|grep "Manifest merge branch"|sed 's/Manifest merge branch: refs\/[a-z]*\///'`
echo "Manifest branch/AU is: $manifest_info"

if [ "$wsp_path" == "" ]
then
    ## this happens when this script runs as part of bitbake compilation in do_compile_prepend
    echo "Wsp path is NULL"
    if [ "$WORKSPACE" != "" ];then
        # LE targets has root path in $WORKSPACE
        wsp_path=$WORKSPACE"/"
    elif [ "$SRC_DIR_ROOT" != "" ];then
        # LV targets has root path in $SRC_DIR_ROOT
        wsp_path=$SRC_DIR_ROOT"/"
    else
        ## Compiling in <ROOT>/poky/build/tmp-glibc/work/sa415m-oe-linux-gnueabi/eng-hub-msg-proto/git-r1/gps-noship/eng-hub-msg-proto/
        ## 9 level up - <ROOT> of workspace
        wsp_path="../../../../../../../../../"
    fi
    echo "do_compile_prepend: Wsp path is $wsp_path"
fi

## LV target
## - <ROOT>/meta-openembedded/meta-oe/recipes-devtools/protobuf/protobuf_3.6.1.bb
## SRCREV = "48cb18e5c419ddd23d9badcfe4e9df7bde1979b2"
## PV .= "+git${SRCPV}"

## LE target
## - <ROOT>/poky/meta-qti-bsp/recipes-devtools/protobuf/protobuf_3.3.0.bb
## SRCREV = "a6189acd18b00611c1dc7042299ad75486f08a1a"
## PV = "3.3.0+git${SRCPV}"

## EAP target
## - <ROOT>/sources/meta-openembedded/meta-oe/recipes-devtools/protobuf/protobuf_3.5.1.bb
## SRCREV = "106ffc04be1abf3ff3399f54ccf149815b287dd9"
## PV = "3.5.1+git${SRCPV}"

## LE-PDK target
## - <ROOT>/poky/meta-qti-bsp/recipes-devtools/protobuf/protobuf_3.3.0.bb
## SRCREV = "a6189acd18b00611c1dc7042299ad75486f08a1a"
## PV = "3.3.0+git${SRCPV}"

if [ -e $wsp_path"sources/meta-qti-eap/recipes-location/location/qcom-gps-utils_git.bb" ]
then
    manifest_info="LE-EAP"
elif [ -e "$wsp_path/../poky/qti-conf/set_bb_env.sh" ]
then
    manifest_info="LE-PDK"
elif [ -e $wsp_path"meta-qti-bsp/meta-qti-base/recipes-location/location/qcom-gps-utils_git.bb" ]
then
    manifest_info="LV"
else
    manifest_info="LE"
fi

protobuf_protoc_path=$(which protoc)
echo "Target is $manifest_info"
echo "Binary protoc path is : $protobuf_protoc_path"
if ! [ -e $protobuf_protoc_path ]
then
    echo "ERROR!! Protoc - $protobuf_protoc_path - is not found..."
    exit 1
fi

echo "Protobuf version is: $(eval $protobuf_protoc_path --version)"

## array of .proto files. Add any new .proto files to this array.
src_proto_files=(LocationApiDataTypes.proto LocationApiMsg.proto)

for src_file in ${src_proto_files[@]}; do
    echo -e "\nProcessing $src_file"
    ## protoc --cpp_out=. EHMsg.proto
    echo " >> Running protoc"
    cmd=$protobuf_protoc_path' --proto_path=. --cpp_out=src '$src_file
    eval $cmd
    if [ $? -eq 0 ]; then
        echo "SUCCESS !!! protoc compilation"
    else
        echo "ERROR!! protoc compilation failed"
        exit 1
    fi
done

#/bin/bash

#Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted (subject to the limitations in the
# disclaimer below) provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#   * Redistributions in binary form must reproduce the above
#     copyright notice, this list of conditions and the following
#     disclaimer in the documentation and/or other materials provided
#     with the distribution.
#
#   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
# GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
# HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

########################################OpenWRT############################################
# protobuf_gen.sh --> Used in OpenWRT Build System
# Description:
# Util script to compile the proto files using protoc to generate the header and C file.
###########################################################################################

protogen_script_path=$(realpath $0)
pkg_build_dir_path=$(dirname $protogen_script_path)
echo "Current directory $pkg_build_dir_path"

protobuf_protoc_path=$1/bin/protoc
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
    echo " >> Running protoc"
    cmd=$protobuf_protoc_path' --proto_path=$pkg_build_dir_path --cpp_out=$pkg_build_dir_path/src '$src_file
    eval $cmd
    if [ $? -eq 0 ]; then
        echo "SUCCESS !!! protoc compilation"
    else
        echo "ERROR!! protoc compilation failed"
        exit 1
    fi
done

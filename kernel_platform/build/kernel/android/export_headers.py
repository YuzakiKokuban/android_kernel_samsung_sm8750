#!/usr/bin/env python

##=============================================================================
# Copyright (c) 2020, The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================
import os
import difflib
import sys
import copy
import shutil
import filecmp

"""
This will generate a set containing all of the UAPI header files under a given
base path for comparison against another set of UAPI headers under another
path.
"""
def generate_hdr_names(base_path):
    hdrs = set()
    for root, dirs, files in os.walk(base_path):
        for full_name in files:
            name, extension = os.path.splitext(full_name)
            if extension == ".h":
                path = os.path.relpath(os.path.join(root, full_name), base_path)
                hdrs.add(path)
    return hdrs

def print_diagnostics(kernel_hdrs, intersection, matches, mismatches, msm_spec_hdrs):
    exports_fd = open("exports.txt", 'w+')
    exports_fd.write("====MSM vs Bionic Header Mismatches====\n")
    for hdr in mismatches: exports_fd.write(hdr + "\n")
    exports_fd.write("\n====MSM Specific Header Files====\n")
    for hdr in msm_spec_hdrs: exports_fd.write(hdr + "\n")
    exports_fd.close()
    total_exports = len(msm_spec_hdrs) + len(mismatches)
    print("====Kernel and Bionic Header Stats====")
    print("Total headers: %d" % len(kernel_hdrs))
    print("Kernel & Bionic common headers: %d matches: %d mismatches: %d" %\
          (len(intersection), len(matches), len(mismatches)))
    print("MSM specific headers: %d" % len(msm_spec_hdrs))
    print("Exporting: %d header files" % total_exports)
    print("MSM Specific headers written to exports.txt")

def print_usage():
    print("Usage: python export_headers.py <MSM_HDRS_PATH> <BIONIC_HDRS_PATH>" +\
          " <PATH_TO_EXPORT_MSM_HDRS> <ARCH>")

"""
This will return the MSM exclusive headers.
"""
def generate_exports(kernel_hdrs_path, bionic_hdrs_path,\
                     final_dest, arch):
    kernel_hdrs = generate_hdr_names(kernel_hdrs_path)
    bionic_hdrs = generate_hdr_names(bionic_hdrs_path)
    bionic_hdrs.update(generate_hdr_names(os.path.join(bionic_hdrs_path, 'asm-' + arch)))
    msm_bionic_intersection = kernel_hdrs.intersection(bionic_hdrs)
    msm_spec_hdrs = kernel_hdrs.difference(bionic_hdrs)
    hdr_matches = set()
    hdr_mismatches = set()
    if not os.path.exists(final_dest):
        os.makedirs(final_dest)
    # If the final_dest folder already exists, we want to remove any "orphaned" headers
    # e.g. existing dest dir had UAPI x/y/z.h, but doesn't exist in new kernel
    dangling_dest_hdrs = generate_hdr_names(final_dest)
    for hdr in msm_spec_hdrs:
        src_hdr = os.path.join(kernel_hdrs_path, hdr)
        dest_hdr = os.path.join(final_dest, hdr)
        dangling_dest_hdrs.discard(hdr)
        if not os.path.exists(os.path.dirname(dest_hdr)):
            os.makedirs(os.path.dirname(dest_hdr))
        # If the file is already in the destination, don't touch it to improve
        # incremental compilation
        if os.path.exists(dest_hdr) and filecmp.cmp(src_hdr, dest_hdr, shallow=False):
            continue
        shutil.copyfile(src_hdr, dest_hdr)

    for hdr in dangling_dest_hdrs:
        if os.path.exists(os.path.join(final_dest, hdr)):
            os.remove(os.path.join(final_dest, hdr))

    print_diagnostics(kernel_hdrs, msm_bionic_intersection, hdr_matches,\
                      hdr_mismatches, msm_spec_hdrs)
    exports = hdr_mismatches.union(msm_spec_hdrs)
    return exports

#Script begins executing here
#KERNEL_HDRS_PATH = Kernel SI UAPI header path
#BIONIC_HDRS_PATH = bionic/libc/kernel/uapi/linux
#FINAL_DEST = Location where the MSM spec headers are to be exported to
#ARCH = arch for asm headers
if len(sys.argv) < 4:
    print_usage()
    sys.exit(1)
else:
    KERNEL_HDRS_PATH = sys.argv[1]
    BIONIC_HDRS_PATH = sys.argv[2]
    FINAL_DEST = sys.argv[3]
    ARCH = sys.argv[4]
export_hdrs = generate_exports(KERNEL_HDRS_PATH, BIONIC_HDRS_PATH,\
                               FINAL_DEST, ARCH)
sys.exit(0)

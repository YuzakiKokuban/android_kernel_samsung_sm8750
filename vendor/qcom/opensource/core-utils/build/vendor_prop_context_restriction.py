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

'''
    This script enforces, at compile time, the naming conventions of vendor defined system properties and their property
    context labels in terms of prefixes. This will also be enforced by VTS; however, the injection of this script at compile
    time will ensure minimal VTS failures of these types.

    Allowed property name prefixes:
                                   ["ctl.odm.",
                                    "ctl.vendor.",
                                    "ctl.start$odm.",
                                    "ctl.start$vendor.",
                                    "ctl.stop$odm.",
                                    "ctl.start$vendor.",
                                    "ro.boot.",
                                    "ro.hardware.",
                                    "ro.odm.",
                                    "ro.vendor.",
                                    "odm.",
                                    "persist.vendor.",
                                    "persist.odm.",
                                    "vendor."
                                   ]
    Ex: vendor.debug.qspm

    Allowed property context label prefixes:
                                           [ "vendor_"
                                             "odm_"
                                           ]
    Ex: vendor_some_prop
'''

import os, sys, argparse

#global constants
OUT = os.getenv("OUT")
if OUT == None:
    print ("No out directory found. skipping check")
    sys.exit(0)

PROPERTY_CONTEXT_FILE_PATH = OUT+ "/vendor/etc/selinux/vendor_property_contexts"
VIOLATORS_OUTPUT_FILE = OUT+"/vendor_prop_context_violators.txt"
ALLOWED_PROP_PREFIXES = ["ctl.odm.",
                       "ctl.vendor.",
                       "ctl.start$odm.",
                       "ctl.start$vendor.",
                       "ctl.stop$odm.",
                       "ctl.start$vendor.",
                       "ro.boot.",
                       "ro.hardware.",
                       "ro.odm.",
                       "ro.vendor.",
                       "odm.",
                       "persist.vendor.",
                       "persist.odm.",
                       "vendor."
                      ]
ALLOWED_CONTEXT_PREFIXES = ["vendor_",
                            "odm_"
                           ]
#global variables
list_of_violator_props = []
arg_parser = argparse.ArgumentParser(description='Vendor defined system properties and contexts naming convention enforcement script')
arg_parser.add_argument('--m', type=str, required=True, choices=["warning","error"],help='Script running mode')
args = arg_parser.parse_args()


def print_msg(list_of_violators, mode):
    if len(list_of_violators) == 0:
        return
    if mode == "error":
        print_list(list_of_violators, "ERROR")
    elif mode == "warning":
        print_list(list_of_violators, "WARNING")

def print_list(list_of_violators, mode):
    print("\n================================================================")
    print("                      "+mode+"                                   ")
    print("================================================================ \n")

    print(mode + ": The following properties violate either property or property context naming convention.\n"\
                    "To learn more about the allowed conventions, look at the commented portion of vendor_prop_context_restriction.py\n")
    for x in list_of_violators:
        print(x)
    if mode == "ERROR": sys.exit(-1)

def check_prop_restriction(prop_name_and_context):
    for x in ALLOWED_PROP_PREFIXES:
        if prop_name_and_context[0].startswith(x):
            return
    list_of_violator_props.append(prop_name_and_context[0] + "\t\t" + prop_name_and_context[1])

def check_context_restriction(prop_name_and_context):
    split_context = prop_name_and_context[1].split(":")
    if not split_context[2].startswith(ALLOWED_CONTEXT_PREFIXES[0]) and \
                     not split_context[2].startswith(ALLOWED_CONTEXT_PREFIXES[1]):
        list_of_violator_props.append(prop_name_and_context[0] + "\t" + prop_name_and_context[1])

def exit_with_warning(reason):
    print ("WARNING: " + reason)
    sys.exit(0)

def main():
    if not os.path.exists(PROPERTY_CONTEXT_FILE_PATH):
        exit_with_warning(PROPERTY_CONTEXT_FILE_PATH + " doesn't exist. skipping check.")

    with open(PROPERTY_CONTEXT_FILE_PATH) as fd:
        line = fd.readline()
        while line:
            if not line.startswith("#") and not line == "\n":
                split_prop_context = line.split()
                check_prop_restriction(split_prop_context)
                check_context_restriction(split_prop_context)
            line = fd.readline()
    fd.close()
    print_msg(list_of_violator_props, args.m)

if __name__ == "__main__":
    main()

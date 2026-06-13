#!/usr/bin/env python

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


import argparse
import fnmatch
import logging
import os
import shutil
import sys
import tempfile
from subprocess import call
from zipfile import ZipFile

'''
Script Versioning:

Version 1.0:
  Supports the following:
  - Generates the Super image from the given Qssi and target builds.
  - Outputs OTA zip as well, optionally.
  - Enforces QIIFA checks, to ensure Qssi and target builds being combined
    and compatible with each other.
Version 1.1:
  - Adds support for --no_tmp option, with which images are fetched directly to
    merged_build_path for merge, instead of using /tmp. Useful on machines
    having very less /tmp storage.
  - Adds check for /tmp free space availablity at script start up.
Version 1.2:
  - Adds support for 32-bit and Go targets
  - Disable qiifa check for 32-bit and Go targets
Verison 1.3:
  - Add python 3 support
  - Add toggle to check python version and change based on python version
'''
__version__ = '1.3'

logger = logging.getLogger(__name__)

BUILD_TOOLS_ZIP  = "buildtools/buildtools.zip"
OUT_DIST         = "out/dist/"
OUT_PREFIX       = "out/target/product/"
OUT_QSSI         = "" # will be set later, as per lunch
OUT_TARGET       = "" # will be set later, as per lunch
QSSI_TARGET      = "" # will be set later, as per lunch
QIIFA_DIR_QSSI   = "QIIFA_QSSI"
QIIFA_DIR_TARGET = "QIIFA_TARGET"

# QSSI build's OUT_DIST artifacts (pattern supported):
QSSI_OUT_DIST_ARTIFACTS = (
    'otatools.zip',
    BUILD_TOOLS_ZIP
)

QSSI_TARGET_FILES_ZIP = "" # will be set later, as per lunch

# TARGET build's OUT_DIST artifacts (pattern supported):
TARGET_OUT_DIST_ARTIFACTS = (
    'merge_config_*',
)

TARGET_TARGET_FILES_ZIP = "-target_files-*.zip"

TARGET_OTATOOLS_ZIP = "otatools.zip"

# MERGED build's OUT_DIST OTA related artifacts that are optionally backed up
# along with super.img via "--output_ota" arg (pattern supported):
BACKUP_MERGED_OUT_DIST_ARTIFACTS = (
    'merge_config_*',
    'merged*-target_files*.zip',
    'merged*-ota*.zip'
)

MIN_TMP_FREE_SIZE_IN_GB = 24

def call_func_with_temp_dir(func, keep_tmp, no_tmp, merged_build_path):
  if no_tmp:
    temp_dir = merged_build_path + "/tmp"
  else:
    temp_dir = tempfile.mkdtemp(prefix='merge_builds_')
  try:
    func(temp_dir)
  finally:
    if keep_tmp:
      logger.info('keeping %s', temp_dir)
    else:
      shutil.rmtree(temp_dir, ignore_errors=True)

def assert_path_readable(path):
  assert os.access(path, os.R_OK), "Path \"" + path + "\" is not readable"

def assert_path_writable(path):
  assert os.access(path, os.W_OK), "Path \"" + path + "\" is not writable"

def copy_items(from_dir, to_dir, files, list_identifier):
  logging.info("Copying " + str(files) + " from \"" + from_dir + "\" to \"" + to_dir + "\"")
  for file_path in files:
    original_file_path = os.path.join(from_dir, file_path)
    copied_file_path = os.path.join(to_dir, file_path)
    copied_file_dir = os.path.dirname(copied_file_path)
    if not os.path.exists(original_file_path):
        logging.error("FAILED: Files: "+ list_identifier + ":" +str(original_file_path)+ " not found")
        sys.exit(1)
    if os.path.isdir(original_file_path):
      if os.path.exists(copied_file_path):
        shutil.rmtree(copied_file_path)
      shutil.copytree(original_file_path, copied_file_path)
    else:
      if os.path.exists(copied_file_path):
        os.remove(copied_file_path)
      if not os.path.exists(copied_file_dir):
        os.makedirs(copied_file_dir)
      shutil.copyfile(original_file_path, copied_file_path)

def copy_pattern_items(from_dir, to_dir, patterns, list_identifier, enforce_single_item_only=False):
  file_paths = []
  for dirpath, _, filenames in os.walk(from_dir):
    file_paths.extend(
        os.path.relpath(path=os.path.join(dirpath, filename), start=from_dir)
        for filename in filenames)

  filtered_file_paths = set()
  for pattern in patterns:
    filtered_file_path = fnmatch.filter(file_paths, pattern)
    if enforce_single_item_only and len(filtered_file_path) > 1:
      logging.error("FAILED: Multiple conflicting files found at " + from_dir  + " for "+ list_identifier + ": " +str(filtered_file_path))
      logging.error("Please ensure there is only one valid file and re-run.")
      sys.exit(1)
    filtered_file_paths.update(filtered_file_path)
  if len(filtered_file_paths) == 0:
    logging.error("FAILED: File: "+ list_identifier + ":" +str(patterns)+ " not found")
    sys.exit(1)
  copy_items(from_dir, to_dir, filtered_file_paths, list_identifier)

def fetch_build_artifacts(temp_dir, qssi_build_path, target_build_path,
                        target_lunch):
  copy_pattern_items(qssi_build_path + "/" + OUT_DIST, temp_dir + "/" + OUT_DIST, QSSI_OUT_DIST_ARTIFACTS, "QSSI_OUT_DIST_ARTIFACTS")
  copy_pattern_items(qssi_build_path + "/" + OUT_DIST, temp_dir + "/" + OUT_DIST, (QSSI_TARGET_FILES_ZIP,), "QSSI_TARGET_FILES_ZIP", True)
  copy_pattern_items(target_build_path + "/" + OUT_DIST, temp_dir + "/" + OUT_DIST, TARGET_OUT_DIST_ARTIFACTS, "TARGET_OUT_DIST_ARTIFACTS")
  copy_pattern_items(target_build_path + "/" + OUT_DIST, temp_dir + "/" + OUT_DIST + "/vendor/", (TARGET_OTATOOLS_ZIP,), "TARGET_OTATOOLS_ZIP")
  copy_pattern_items(target_build_path + "/" + OUT_DIST, temp_dir + "/" + OUT_DIST, (target_lunch + TARGET_TARGET_FILES_ZIP,), "TARGET_TARGET_FILES_ZIP", True)
  with ZipFile(temp_dir + "/" + OUT_DIST + BUILD_TOOLS_ZIP, 'r') as zipObj:
    zipObj.extractall(temp_dir)

def qiifa_abort(error_msg):
    logging.error(error_msg)
    logging.error("Use --skip_qiifa argument to intentionally skip Qiifa checking (but this may just defer the real Qssi and target incompatibility issues until later)")
    sys.exit(1)

def run_qiifa_checks(temp_dir, qssi_build_path, target_build_path, merged_build_path, target_lunch):
  logging.info("Starting QIIFA checks (to determine if the builds are compatible with each other):")

  global OUT_TARGET

  QIIFA_CHECKS_DIR = "QIIFA_CHECKS_DIR"
  QIIFA_CHECKS_DIR_PATH = temp_dir + "/" + QIIFA_CHECKS_DIR + "/"
  QIIFA_CHECKS_DIR_PATH_QSSI = QIIFA_CHECKS_DIR_PATH + "qssi"
  QIIFA_CHECKS_DIR_PATH_TARGET = QIIFA_CHECKS_DIR_PATH + target_lunch

  # Fetch the QIIFA script
  python_version = int(sys.version_info.major)
  QIIFA_SCRIPT = ""
  if python_version == 2:
      QIIFA_SCRIPT = "qiifa_py2"
  elif python_version == 3:
      QIIFA_SCRIPT = "qiifa_py3"
  else:
      QIIFA_SCRIPT = "qiifa_py3"
  if os.path.exists(qssi_build_path + "/" + OUT_QSSI + QIIFA_DIR_QSSI + "/" + QIIFA_SCRIPT):
    # Check for QIIFA script from $OUT_QSSI/QIIFA path first
    copy_items(qssi_build_path + "/" + OUT_QSSI + QIIFA_DIR_QSSI + "/", QIIFA_CHECKS_DIR_PATH, [QIIFA_SCRIPT], "QIIFA_SCRIPT")
  elif os.path.exists(qssi_build_path + "/out/host/linux-x86/bin/" + QIIFA_SCRIPT):
    # Check for QIIFA script from host path if above one is not found
    copy_items(qssi_build_path + "/out/host/linux-x86/bin/", QIIFA_CHECKS_DIR_PATH, [QIIFA_SCRIPT], "QIIFA_SCRIPT")
  else:
    qiifa_abort("QIIFA script: " + QIIFA_SCRIPT + " not found !")

  # Copy QIIFA cmds:
  QIIFA_DIR_QSSI_PATH = qssi_build_path + "/" + OUT_QSSI + QIIFA_DIR_QSSI
  if not os.path.exists(QIIFA_DIR_QSSI_PATH):
    qiifa_abort("QIIFA cmd on QSSI side not found: " + QIIFA_DIR_QSSI_PATH)
  else:
    copy_items(qssi_build_path + "/" + OUT_QSSI, QIIFA_CHECKS_DIR_PATH_QSSI, [QIIFA_DIR_QSSI], "QIIFA_QSSI")

  QIIFA_DIR_TARGET_PATH = target_build_path + "/" + OUT_TARGET + "/" + QIIFA_DIR_TARGET
  if not os.path.exists(QIIFA_DIR_TARGET_PATH):
    qiifa_abort("QIIFA cmd on Target side not found: " + QIIFA_DIR_TARGET_PATH)
  else:
    copy_items(target_build_path + "/" + OUT_TARGET, QIIFA_CHECKS_DIR_PATH_TARGET, [QIIFA_DIR_TARGET], "QIIFA_TARGET")

  # Run QIIFA
  os.chdir(QIIFA_CHECKS_DIR_PATH)
  cmd = ["python", QIIFA_SCRIPT, "--qssi", "qssi", "--target", target_lunch]
  logging.info("Running: " + str(cmd))
  status = call(cmd)

  if status == 0:
    logging.info("QIIFA checks Passed, builds are compatible !")
    copy_items(temp_dir, merged_build_path, [QIIFA_CHECKS_DIR], "QIIFA_CHECKS_DIR_BACKUP")
  else:
    qiifa_abort("QIIFA checks failed, Qssi and Target builds not compatible, aborting.")

def build_superimage(temp_dir, qssi_build_path, target_build_path,
                 merged_build_path, target_lunch, output_ota, skip_qiifa, no_tmp):
  logging.info("Script Version : " + __version__)
  logging.info("Starting up builds merge..")
  logging.info("QSSI build path = " + qssi_build_path)
  logging.info("Target build path = " + target_build_path)
  logging.info("Merged build path = " + merged_build_path)

  if not no_tmp:
    # Ensure there is free space on /tmp
    statvfs = os.statvfs('/tmp')
    tmp_free_space = float(statvfs.f_frsize * statvfs.f_bavail)/(1024*1024*1024)
    logging.info("Free Space available on /tmp = " + str(tmp_free_space) + "G")

    if tmp_free_space < MIN_TMP_FREE_SIZE_IN_GB:
      logging.error("Not enough free space available on /tmp, aborting, min free space required = " + str(MIN_TMP_FREE_SIZE_IN_GB) + "G !!")
      logging.error("Free up /tmp manually, and/or Increase it using: sudo mount -o remount,size=" + str(MIN_TMP_FREE_SIZE_IN_GB) + "G" + " tmpfs /tmp")
      logging.error("Or Alternatively, Use --no_tmp option while triggering build_image_standalone, to not use /tmp if it is a low RAM machine")
      sys.exit(1)

  global OUT_TARGET
  OUT_TARGET = OUT_PREFIX + target_lunch

  # Ensure paths are readable/writable
  assert_path_readable(qssi_build_path)
  assert_path_readable(target_build_path)
  if not os.path.exists(merged_build_path):
    os.makedirs(merged_build_path)
  assert_path_writable(merged_build_path)

  # Run QIIFA checks to ensure these builds are compatible, before merging them.
  if not skip_qiifa:
    if QSSI_TARGET == "qssi" or QSSI_TARGET == "qssi_64" :
      run_qiifa_checks(temp_dir, qssi_build_path, target_build_path, merged_build_path, target_lunch)
    else:
      logging.info("Skipping QIIFA checks for 32-bit and Go targets")

  # Fetch the build artifacts to temp dir
  fetch_build_artifacts(temp_dir, qssi_build_path, target_build_path,
                        target_lunch)
  # Setup environment
  logging.info("Setting up environment...")
  os.environ["TARGET_PRODUCT"] = target_lunch
  OUT_PATH = "out/target/product/" + target_lunch
  os.environ["OUT"] = merged_build_path + "/" + OUT_PATH
  if not os.path.exists(os.environ["OUT"]):
    os.makedirs(os.environ["OUT"])
  # Setup Java Path
  with open(temp_dir + "/JAVA_HOME.txt") as fp:
    JAVA_PREBUILT_PATH = fp.readline().strip()
  os.environ["PATH"] = temp_dir + "/" + JAVA_PREBUILT_PATH + "/bin" + ":" + os.environ["PATH"]

  if no_tmp:
    os.environ["TMPDIR"] = os.environ["TMP"] = os.environ["TEMP"] = temp_dir

  os.chdir(temp_dir)
  # Ensure java bins have execute permission
  cmd = ["chmod", "+x", "-R", JAVA_PREBUILT_PATH]
  status = call(cmd)

  logging.info("Triggering Merge Process and generating merged-target-files, OTA zip and super.img...")
  cmd = ["bash", "vendor/qcom/opensource/core-utils/build/build.sh", "dist", "-j16", "--merge_only", "--rebuild_sepolicy_with_vendor_otatools=out/dist/vendor/"+TARGET_OTATOOLS_ZIP]
  logging.info("Running: " + str(cmd))
  status = call(cmd)

  if status != 0:
   logging.error("FAILED:" + str(cmd))
   sys.exit(1)

  if output_ota:
    copy_pattern_items(temp_dir + "/" + OUT_DIST, merged_build_path + "/" + OUT_DIST, BACKUP_MERGED_OUT_DIST_ARTIFACTS, "BACKUP_MERGED_OUT_DIST_ARTIFACTS")

def main():
  logging_format = '%(asctime)s - %(filename)s - %(levelname)-8s: %(message)s'
  logging.basicConfig(level=logging.INFO, format=logging_format, datefmt='%Y-%m-%d %H:%M:%S')

  parser = argparse.ArgumentParser()
  parser.add_argument("--image", dest='image',
                    help="Image to be built", required=True)
  parser.add_argument("--qssi_build_path", dest='qssi_build_path',
                      help="QSSI build path", required=True)
  parser.add_argument("--target_build_path", dest='target_build_path',
                    help="Target build path", required=True)
  parser.add_argument("--merged_build_path", dest='merged_build_path',
                    help="Merged build path", required=True)
  parser.add_argument("--target_lunch", dest='target_lunch',
                    help="Target lunch name", required=True)
  parser.add_argument("--keep_tmp",  dest='keep_tmp',
                    help="Keep tmp dir for debugging", action='store_true')
  parser.add_argument("--output_ota",  dest='output_ota',
                    help="Outputs OTA related zips additionally", action='store_true')
  parser.add_argument("--skip_qiifa",  dest='skip_qiifa',
                    help="Skips QIIFA checks (but this may just defer the real Qssi and target incompatibility issues until later)", action='store_true')
  parser.add_argument("--no_tmp",  dest='no_tmp',
                    help="Doesn't use tmp, instead uses merged_build_path to fetch images and merge", action='store_true')
  parser.add_argument('--version', action='version', version=__version__)

  args = parser.parse_args()

  global QSSI_TARGET, OUT_QSSI, QSSI_TARGET_FILES_ZIP

  # use this dict to pair target lunch options with their corresponding qssi type.
  vendor_qssi_mapping_dict = {
    "qssi"      : ["holi", "taro", "kalama", "lahaina", "sdm710", "sdm845", "msmnile", "sm6150", "kona", "atoll", "trinket", "lito", "bengal", "parrot", "bengal_515", "crow", "anorak"],
    "qssi_32"   : ["bengal_32"],
    "qssi_32go" : ["bengal_32go", "msm8937_lily","pitti_32go" ],
    "qssi_64"   : ["kalama64", "pineapple", "blair", "hala", "sun", "niobe", "parrot66", "volcano", "canoe","pitti"],
  }


  if args.target_lunch   in vendor_qssi_mapping_dict['qssi']:
    QSSI_TARGET="qssi"
  elif args.target_lunch in vendor_qssi_mapping_dict['qssi_32go']:
    QSSI_TARGET="qssi_32go"
  elif args.target_lunch in vendor_qssi_mapping_dict['qssi_32']:
    QSSI_TARGET="qssi_32"
  elif args.target_lunch in vendor_qssi_mapping_dict['qssi_64']:
    QSSI_TARGET="qssi_64"
  else:
    print("ERROR: Unrecognized target_lunch input. Need to add lunch option to the vendor_qssi_matching_dict")
    return

  OUT_QSSI = OUT_PREFIX + QSSI_TARGET + "/"
  QSSI_TARGET_FILES_ZIP = QSSI_TARGET + "-target_files-*.zip"

  if args.image == "super":
    call_func_with_temp_dir(
        lambda temp_dir: build_superimage(
            temp_dir=temp_dir,
            qssi_build_path=os.path.abspath(args.qssi_build_path),
            target_build_path=os.path.abspath(args.target_build_path),
            merged_build_path=os.path.abspath(args.merged_build_path),
            target_lunch=args.target_lunch,
            output_ota=args.output_ota,
            skip_qiifa=args.skip_qiifa,
            no_tmp=args.no_tmp), args.keep_tmp, args.no_tmp, os.path.abspath(args.merged_build_path))
  else:
    logging.error("No support to build \"" + args.image + "\" image, exiting..")
  logging.info("Completed Successfully!")

if __name__ == '__main__':
  main()

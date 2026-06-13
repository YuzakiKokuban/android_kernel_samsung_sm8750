#!/bin/bash

# Copyright (C) 2021 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# $1 directory of kernel modules ($1/lib/modules/x.y)
# $2 flags to pass to depmod
# $3 kernel version
# $4 Optional: File with list of modules to run depmod on.
#              If left empty, depmod will run on all modules
#              under $1/lib/modules/x.y
function run_depmod() {
  (
    local ramdisk_dir=$1
    local depmod_stdout
    local depmod_stderr=$(mktemp)
    local version=$3
    local modules_list_file=$4
    local modules_list=""

    if [[ -n "${modules_list_file}" ]]; then
      while read -r line; do
        # depmod expects absolute paths for module files
        modules_list+="$(realpath ${ramdisk_dir}/lib/modules/${version}/${line}) "
      done <${modules_list_file}
    fi

    cd ${ramdisk_dir}
    if ! depmod_stdout="$(depmod $2 -F ${DIST_DIR}/System.map -b . ${version} ${modules_list} \
        2>${depmod_stderr})"; then
      echo "$depmod_stdout"
      cat ${depmod_stderr} >&2
      rm -f ${depmod_stderr}
      exit 1
    fi
    [ -n "$depmod_stdout" ] && echo "$depmod_stdout"
    cat ${depmod_stderr} >&2
    if { grep -q "needs unknown symbol" ${depmod_stderr}; }; then
      echo "ERROR: kernel module(s) need unknown symbol(s)" >&2
      rm -f ${depmod_stderr}
      exit 1
    fi
    rm -f ${depmod_stderr}
  )
}

# $1 MODULES_LIST, <File containing the list of modules that should go in the
#                   ramdisk.>
# $2 MODULES_RECOVERY_LIST, <File containing the list of modules that should
#                            go in the ramdisk and be loaded when booting into
#                            recovery mode during first stage init.
#
#                            This parameter is optional, and if not used, should
#                            be passed as an empty string to ensure that
#                            subsequent parameters are treated correctly.>
# $3 MODULES_CHARGER_LIST, <File containing the list of modules that should
#                           go in the ramdisk and be loaded when booting into
#                           charger mode during first stage init.
#
#                           This parameter is optional, and if not used, should
#                           be passed as an empty string to ensure that
#                           subsequent paratmers are treated correctly.>
# $4 MODULES_ORDER_LIST, <The modules.order file that contains all of the
#                         modules that were built.>
#
# This function creates new modules.order* files by filtering the module lists
# through the set of modules that were built ($MODULES_ORDER_LIST).
#
# Each modules.order* file is created by filtering each list as follows:
#
# Let f be the filter_module_list function, which filters arg 1 through arg 2.
#
# f(MODULES_LIST, MODULES_ORDER_LIST) ==> modules.order
#
# f(MODULES_RECOVERY_LIST, MODULES_ORDER_LIST) ==> modules.order.recovery
#
# f(MODULES_CHARGER_LIST, MODULES_ORDER_LIST) ==> modules.order.charger
#
# Filtering ensures that only the modules in MODULES_LIST end up in the
# respective partition that create_modules_staging() is invoked for.
#
# Note: This function overwrites the original file pointed to by
# MODULES_ORDER_LIST when MODULES_LIST is set.
function create_modules_order_lists() {
  local modules_list_file="${1}"
  local modules_recovery_list_file="${2}"
  local modules_charger_list_file="${3}"
  local modules_order_list_file="${4}"
  local dest_dir=$(dirname $(realpath ${modules_order_list_file}))
  local tmp_modules_order_file=$(mktemp)

  cp ${modules_order_list_file} ${tmp_modules_order_file}

  declare -A module_lists_arr
  module_lists_arr["modules.order"]=${modules_list_file}
  module_lists_arr["modules.order.recovery"]=${modules_recovery_list_file}
  module_lists_arr["modules.order.charger"]=${modules_charger_list_file}

  for mod_order_file in ${!module_lists_arr[@]}; do
    local mod_list_file=${module_lists_arr[${mod_order_file}]}
    local dest_file=${dest_dir}/${mod_order_file}

    # Need to make sure we can find modules_list_file from the staging dir
    if [[ -n "${mod_list_file}" ]]; then
      if [[ -f "${ROOT_DIR}/${mod_list_file}" ]]; then
        modules_list_file="${ROOT_DIR}/${mod_list_file}"
      elif [[ "${mod_list_file}" != /* ]]; then
        echo "ERROR: modules list must be an absolute path or relative to ${ROOT_DIR}: ${mod_list_file}" >&2
        rm -f ${tmp_modules_order_file}
        exit 1
      elif [[ ! -f "${mod_list_file}" ]]; then
        echo "ERROR: Failed to find modules list: ${mod_list_file}" >&2
        rm -f ${tmp_modules_order_file}
        exit 1
      fi

      local modules_list_filter=$(mktemp)

      # Remove all lines starting with "#" (comments)
      # Exclamation point makes interpreter ignore the exit code under set -e
      ! grep -v "^#" ${mod_list_file} > ${modules_list_filter}

      # Append a new line at the end of file
      # If file doesn't end in newline the last module is skipped from filter
      echo >> ${modules_list_filter}

      # grep the modules.order for any KOs in the modules list
      ! grep -w -f ${modules_list_filter} ${tmp_modules_order_file} > ${dest_file}

      rm -f ${modules_list_filter}
    fi
  done

  rm -f ${tmp_modules_order_file}
}

# $1 MODULES_LIST, <File contains the list of modules that should go in the ramdisk>
# $2 MODULES_STAGING_DIR    <The directory to look for all the compiled modules>
# $3 IMAGE_STAGING_DIR  <The destination directory in which MODULES_LIST is
#                        expected, and it's corresponding modules.* files>
# $4 MODULES_BLOCKLIST, <File contains the list of modules to prevent from loading>
# $5 MODULES_RECOVERY_LIST <File contains the list of modules that should go in
#                           the ramdisk but should only be loaded when booting
#                           into recovery.
#
#                           This parameter is optional, and if not used, should
#                           be passed as an empty string to ensure that the depmod
#                           flags are assigned correctly.>
# $6 MODULES_CHARGER_LIST <File contains the list of modules that should go in
#                          the ramdisk but should only be loaded when booting
#                          into charger mode.
#
#                          This parameter is optional, and if not used, should
#                          be passed as an empty string to ensure that the
#                          depmod flags are assigned correctly.>
# $7 flags to pass to depmod
function create_modules_staging() {
  local modules_list_file=$1
  local src_dir=$(echo $2/lib/modules/*)
  local version=$(basename "${src_dir}")
  local dest_dir=$3/lib/modules/${version}
  local dest_stage=$3
  local modules_blocklist_file=$4
  local modules_recovery_list_file=$5
  local modules_charger_list_file=$6
  local depmod_flags=$7

  rm -rf ${dest_dir}
  mkdir -p ${dest_dir}/kernel
  find ${src_dir}/kernel/ -maxdepth 1 -mindepth 1 \
    -exec cp -r {} ${dest_dir}/kernel/ \;
  # The other modules.* files will be generated by depmod
  cp ${src_dir}/modules.order ${dest_dir}/modules.order
  cp ${src_dir}/modules.builtin ${dest_dir}/modules.builtin
  cp ${src_dir}/modules.builtin.modinfo ${dest_dir}/modules.builtin.modinfo

  if [[ -n "${KLEAF_MODULES_ORDER}" ]] && [[ -d "${src_dir}/extra" ]]; then
    mkdir -p ${dest_dir}/extra/
    cp -r ${src_dir}/extra/* ${dest_dir}/extra/
    cat ${KLEAF_MODULES_ORDER} >> ${dest_dir}/modules.order
  elif [[ -n "${EXT_MODULES}" ]] || [[ -n "${EXT_MODULES_MAKEFILE}" ]]; then
    mkdir -p ${dest_dir}/extra/
    cp -r ${src_dir}/extra/* ${dest_dir}/extra/

    # Check if we have modules.order files for external modules. This is
    # supported in android-mainline since 5.16 and androidX-5.15
    FIND_OUT=$(find ${dest_dir}/extra -name modules.order.* -print -quit)
    if [[ -n "${EXT_MODULES}" ]] && [[ "${FIND_OUT}" =~ modules.order ]]; then
      # If EXT_MODULES is defined and we have modules.order.* files for
      # external modules, then we should follow this module load order:
      #   1) Load modules in order defined by EXT_MODULES.
      #   2) Within a given external module, load in order defined by
      #      modules.order.
      for EXT_MOD in ${EXT_MODULES}; do
        # Since we set INSTALL_MOD_DIR=extra/${EXTMOD}, we can directly use the
        # modules.order.* file at that path instead of tring to figure out the
        # full name of the modules.order file. This is complicated because we
        # set M=... to a relative path which can't easily be calculated here
        # when using kleaf due to sandboxing.
        modules_order_file=$(ls ${dest_dir}/extra/${EXT_MOD}/modules.order.*)
        if [[ -f "${modules_order_file}" ]]; then
          cat ${modules_order_file} >> ${dest_dir}/modules.order
        else
          # We need to fail here; otherwise, you risk the module(s) not getting
          # included in modules.load.
          echo "ERROR: Failed to find ${modules_order_file}" >&2
          exit 1
        fi
      done
    else
      # TODO: can we retain modules.order when using EXT_MODULES_MAKEFILE? For
      # now leave this alone since EXT_MODULES_MAKEFILE isn't support in v5.13+.
      (cd ${dest_dir}/ && \
        find extra -type f -name "*.ko" | sort >> modules.order)
    fi
  fi

  if [ "${DO_NOT_STRIP_MODULES}" = "1" ]; then
    # strip debug symbols off initramfs modules
    find ${dest_dir} -type f -name "*.ko" \
      -exec ${OBJCOPY:-${CROSS_COMPILE}objcopy} --strip-debug {} \;
  fi

  # create_modules_order_lists() will overwrite modules.order if MODULES_LIST
  # is set. If we are not trimming unused modules, then backup the original
  # modules.order file first so we can restore it later once the modules.load
  # file is generated. This allows us to account for all the modules that were
  # compiled for later use of the staging archive.
  if [ -z "${TRIM_UNUSED_MODULES}" ]; then
    cp -f ${dest_dir}/modules.order ${dest_dir}/modules.order.orig
  fi
  create_modules_order_lists "${modules_list_file:-""}" "${modules_recovery_list_file:-""}" \
	                     "${modules_charger_list_file:-""}" ${dest_dir}/modules.order

  if [ -n "${modules_blocklist_file}" ]; then
    # Need to make sure we can find modules_blocklist_file from the staging dir
    if [[ -f "${ROOT_DIR}/${modules_blocklist_file}" ]]; then
      modules_blocklist_file="${ROOT_DIR}/${modules_blocklist_file}"
    elif [[ "${modules_blocklist_file}" != /* ]]; then
      echo "ERROR: modules blocklist must be an absolute path or relative to ${ROOT_DIR}: ${modules_blocklist_file}" >&2
      exit 1
    elif [[ ! -f "${modules_blocklist_file}" ]]; then
      echo "ERROR: Failed to find modules blocklist: ${modules_blocklist_file}" >&2
      exit 1
    fi

    cp ${modules_blocklist_file} ${dest_dir}/modules.blocklist
  fi

  if [ -n "${TRIM_UNUSED_MODULES}" ]; then
    local used_blocklist_modules=$(mktemp)
    if [ -f ${dest_dir}/modules.blocklist ]; then
      # TODO: the modules blocklist could contain module aliases instead of the filename
      sed -n -E -e 's/blocklist (.+)/\1/p' ${dest_dir}/modules.blocklist > $used_blocklist_modules
    fi

    # Remove modules from tree that aren't mentioned in modules.order
    (
      cd ${dest_dir}
      local grep_flags="-v -w -f modules.order -f ${used_blocklist_modules} "
      if [[ -f modules.order.recovery ]]; then
        grep_flags+="-f modules.order.recovery "
      fi
      if [[ -f modules.order.charger ]]; then
        grep_flags+="-f modules.order.charger "
      fi
      find * -type f -name "*.ko" | (grep ${grep_flags} - || true) | xargs -r rm
    )
    rm $used_blocklist_modules
  fi

  # Re-run depmod to detect any dependencies between in-kernel and external
  # modules, as well as recovery and charger modules. Then, create the
  # modules.order files based on all the modules compiled.
  #
  # It is important that "modules.order" is last, as that will force depmod
  # to run on all the modules in the directory, instead of just a list of them.
  # It is desirable for depmod to run with all the modules last so that the
  # dependency information is available for all modules, not just the recovery
  # or charger sets.
  modules_order_files=("modules.order.recovery" "modules.order.charger" "modules.order")
  modules_load_files=("modules.load.recovery" "modules.load.charger" "modules.load")

  for i in ${!modules_order_files[@]}; do
    local mod_order_filepath=${dest_dir}/${modules_order_files[$i]}
    local mod_load_filepath=${dest_dir}/${modules_load_files[$i]}

    if [[ -f ${mod_order_filepath} ]]; then
      if [[ "${modules_order_files[$i]}" == "modules.order" ]]; then
        run_depmod ${dest_stage} "${depmod_flags}" "${version}"
      else
        run_depmod ${dest_stage} "${depmod_flags}" "${version}" "${mod_order_filepath}"
      fi
      cp ${mod_order_filepath} ${mod_load_filepath}
    fi
  done

  if [ -z "${TRIM_UNUSED_MODULES}" ]; then
    # Restore the original modules.order file if we didn't trim the unused
    # modules. This allows us to account for all the modules that were compiled
    # for later use of the staging archive.
    mv -f ${dest_dir}/modules.order.orig ${dest_dir}/modules.order
  fi
}

function build_flattened_dlkm_image() {
  # $1 - image name - either vendor_dlkm.flattened.img or system_dlkm.flattened.img
  # $2 - staging dir
  # $3 - props file
  # $4 - dist_dir

  local image_name=$1
  local staging_dir=$2
  local props_file=$3
  local dist_dir=$4
  local image_type

  if [[ "${image_name}" =~ "vendor" ]]; then
    image_type="vendor"
  elif [[ "${image_name}" =~ "system" ]]; then
    image_type="system"
  else
    echo "ERROR: Unknown flattened image type: $1"
    exit 1
  fi

  mkdir -p ${staging_dir}/flatten/lib/modules
  cp $(find ${staging_dir} -type f -name "*.ko") ${staging_dir}/flatten/lib/modules
  # Copy required depmod artifacts and scrub required files to correct paths
  cp $(find ${staging_dir} -name "modules.dep") ${staging_dir}/flatten/lib/modules
  # Copy modules aliases definitions
  cp $(find ${staging_dir} -name "modules.alias") ${staging_dir}/flatten/lib/modules
  # Remove existing paths leaving just basenames
  sed -i 's/kernel[^:[:space:]]*\/\([^:[:space:]]*\.ko\)/\1/g' ${staging_dir}/flatten/lib/modules/modules.dep
  # Prefix /system/lib/modules/ for every module
  sed -i "s#\([^:[:space:]]*\.ko\)#/${image_type}/lib/modules/\1#g" ${staging_dir}/flatten/lib/modules/modules.dep
  cp $(find ${staging_dir} -name "modules.load") ${staging_dir}/flatten/lib/modules
  sed -i 's#.*/##' ${staging_dir}/flatten/lib/modules/modules.load

  build_image "${staging_dir}/flatten" "${props_file}" \
  "${dist_dir}/${image_name}" /dev/null

}

function build_system_dlkm() {
  rm -rf ${SYSTEM_DLKM_STAGING_DIR}
  # MODULES_[RECOVERY_LIST|CHARGER]_LIST should not influence system_dlkm, as
  # GKI modules are not loaded when booting into either recovery or charger
  # modes, so do not consider them, and pass empty strings instead.
  create_modules_staging "${SYSTEM_DLKM_MODULES_LIST:-${MODULES_LIST}}" "${MODULES_STAGING_DIR}" \
    ${SYSTEM_DLKM_STAGING_DIR} "${SYSTEM_DLKM_MODULES_BLOCKLIST:-${MODULES_BLOCKLIST}}" \
    "" "" "-e"

  local system_dlkm_root_dir=$(echo ${SYSTEM_DLKM_STAGING_DIR}/lib/modules/*)
  cp ${system_dlkm_root_dir}/modules.load ${DIST_DIR}/system_dlkm.modules.load
  local system_dlkm_props_file
  local system_dlkm_file_contexts

  if [ -f "${system_dlkm_root_dir}/modules.blocklist" ]; then
    cp "${system_dlkm_root_dir}/modules.blocklist" "${DIST_DIR}/system_dlkm.modules.blocklist"
  fi

  local system_dlkm_default_fs_type="ext4"
  if [[ "${SYSTEM_DLKM_FS_TYPE}" != "ext4" && "${SYSTEM_DLKM_FS_TYPE}" != "erofs" ]]; then
    echo "WARNING: Invalid SYSTEM_DLKM_FS_TYPE = ${SYSTEM_DLKM_FS_TYPE}" >&2
    SYSTEM_DLKM_FS_TYPE="${system_dlkm_default_fs_type}"
    echo "INFO: Defaulting SYSTEM_DLKM_FS_TYPE to ${SYSTEM_DLKM_FS_TYPE}"
  fi

  if [ -z "${SYSTEM_DLKM_PROPS}" ]; then
    system_dlkm_props_file="$(mktemp)"
    system_dlkm_file_contexts="$(mktemp)"
    echo -e "fs_type=${SYSTEM_DLKM_FS_TYPE}\n" >> ${system_dlkm_props_file}
    echo -e "use_dynamic_partition_size=true\n" >> ${system_dlkm_props_file}
    if [[ "${SYSTEM_DLKM_FS_TYPE}" == "ext4" ]]; then
      echo -e "ext_mkuserimg=mkuserimg_mke2fs\n" >> ${system_dlkm_props_file}
      echo -e "ext4_share_dup_blocks=true\n" >> ${system_dlkm_props_file}
      echo -e "extfs_rsv_pct=0\n" >> ${system_dlkm_props_file}
      echo -e "journal_size=0\n" >> ${system_dlkm_props_file}
    fi
    echo -e "mount_point=system_dlkm\n" >> ${system_dlkm_props_file}
    echo -e "selinux_fc=${system_dlkm_file_contexts}\n" >> ${system_dlkm_props_file}

    echo -e "/system_dlkm(/.*)? u:object_r:system_dlkm_file:s0" > ${system_dlkm_file_contexts}
  else
    system_dlkm_props_file="${SYSTEM_DLKM_PROPS}"
    if [[ -f "${ROOT_DIR}/${system_dlkm_props_file}" ]]; then
      system_dlkm_props_file="${ROOT_DIR}/${system_dlkm_props_file}"
    elif [[ "${system_dlkm_props_file}" != /* ]]; then
      echo "ERROR: SYSTEM_DLKM_PROPS must be an absolute path or relative to ${ROOT_DIR}: ${system_dlkm_props_file}" >&2
      exit 1
    elif [[ ! -f "${system_dlkm_props_file}" ]]; then
      echo "ERROR: Failed to find SYSTEM_DLKM_PROPS: ${system_dlkm_props_file}" >&2
      exit 1
    fi
  fi

  # Re-sign the stripped modules using kernel build time key
  # If SYSTEM_DLKM_RE_SIGN=0, this is a trick in Kleaf for building
  # device-specific system_dlkm image, where keys are not available but the
  # signed and stripped modules are in MODULES_STAGING_DIR.
  if [[ ${SYSTEM_DLKM_RE_SIGN:-1} == "1" ]]; then
    for module in $(find ${SYSTEM_DLKM_STAGING_DIR} -type f -name "*.ko"); do
      ${OUT_DIR}/scripts/sign-file sha1 \
      ${OUT_DIR}/certs/signing_key.pem \
      ${OUT_DIR}/certs/signing_key.x509 "${module}"
    done
  fi

  if [ -z "${SYSTEM_DLKM_IMAGE_NAME}" ]; then
    SYSTEM_DLKM_IMAGE_NAME="system_dlkm.img"
  fi

  build_image "${SYSTEM_DLKM_STAGING_DIR}" "${system_dlkm_props_file}" \
    "${DIST_DIR}/${SYSTEM_DLKM_IMAGE_NAME}" /dev/null
  local generated_images=(${SYSTEM_DLKM_IMAGE_NAME})

  # Build flatten image as /lib/modules/*.ko; if unset or null: default false
  if [[ ${SYSTEM_DLKM_GEN_FLATTEN_IMAGE:-0} == "1" ]]; then
    local system_dlkm_flatten_image_name="system_dlkm.flatten.${SYSTEM_DLKM_FS_TYPE}.img"

    build_flattened_dlkm_image "${system_dlkm_flatten_image_name}" "${SYSTEM_DLKM_STAGING_DIR}" \
      "${system_dlkm_props_file}" "${DIST_DIR}"

    generated_images+=(${system_dlkm_flatten_image_name})
   fi

  if [ -z "${SYSTEM_DLKM_PROPS}" ]; then
    rm ${system_dlkm_props_file}
    rm ${system_dlkm_file_contexts}
  fi

  # No need to sign the image as modules are signed
  for image in "${generated_images[@]}"
  do
    avbtool add_hashtree_footer \
      --partition_name system_dlkm \
      --hash_algorithm sha256 \
      --image "${DIST_DIR}/${image}"
  done

  # Archive system_dlkm_staging_dir
  tar -czf "${DIST_DIR}/system_dlkm_staging_archive.tar.gz" -C "${SYSTEM_DLKM_STAGING_DIR}" .
}

# $1 if set, generate the vendor_dlkm_staging_archive.tar.gz archive
function build_vendor_dlkm() {
  local vendor_dlkm_archive=$1

  create_modules_staging "${VENDOR_DLKM_MODULES_LIST}" "${MODULES_STAGING_DIR}" \
    "${VENDOR_DLKM_STAGING_DIR}" "${VENDOR_DLKM_MODULES_BLOCKLIST}"

  local vendor_dlkm_modules_root_dir=$(echo ${VENDOR_DLKM_STAGING_DIR}/lib/modules/*)
  local vendor_dlkm_modules_load=${vendor_dlkm_modules_root_dir}/modules.load
  if [ -f ${vendor_dlkm_modules_root_dir}/modules.blocklist ]; then
    cp ${vendor_dlkm_modules_root_dir}/modules.blocklist ${DIST_DIR}/vendor_dlkm.modules.blocklist
  fi

  # Modules loaded in vendor_boot (and optionally system_dlkm if dedup_dlkm_modules)
  # should not be loaded in vendor_dlkm.
  if [ -f ${DIST_DIR}/modules.load ]; then
    local stripped_modules_load="$(mktemp)"
    ! grep -x -v -F -f ${DIST_DIR}/modules.load \
      ${vendor_dlkm_modules_load} > ${stripped_modules_load}
    mv -f ${stripped_modules_load} ${vendor_dlkm_modules_load}
  fi

  cp ${vendor_dlkm_modules_load} ${DIST_DIR}/vendor_dlkm.modules.load

  if [ -e ${vendor_dlkm_modules_root_dir}/modules.blocklist ]; then
    cp ${vendor_dlkm_modules_root_dir}/modules.blocklist \
      ${DIST_DIR}/vendor_dlkm.modules.blocklist
  fi

  local vendor_dlkm_props_file

  local vendor_dlkm_default_fs_type="ext4"
  if [[ "${VENDOR_DLKM_FS_TYPE}" != "ext4" && "${VENDOR_DLKM_FS_TYPE}" != "erofs" ]]; then
    echo "WARNING: Invalid VENDOR_DLKM_FS_TYPE = ${VENDOR_DLKM_FS_TYPE}" >&2
    VENDOR_DLKM_FS_TYPE="${vendor_dlkm_default_fs_type}"
    echo "INFO: Defaulting VENDOR_DLKM_FS_TYPE to ${VENDOR_DLKM_FS_TYPE}"
  fi

  if [ -z "${VENDOR_DLKM_PROPS}" ]; then
    vendor_dlkm_props_file="$(mktemp)"
    echo -e "vendor_dlkm_fs_type=${VENDOR_DLKM_FS_TYPE}\n" >> ${vendor_dlkm_props_file}
    echo -e "use_dynamic_partition_size=true\n" >> ${vendor_dlkm_props_file}
    if [[ "${VENDOR_DLKM_FS_TYPE}" == "ext4" ]]; then
      echo -e "ext_mkuserimg=mkuserimg_mke2fs\n" >> ${vendor_dlkm_props_file}
      echo -e "ext4_share_dup_blocks=true\n" >> ${vendor_dlkm_props_file}
    fi
  else
    vendor_dlkm_props_file="${VENDOR_DLKM_PROPS}"
    if [[ -f "${ROOT_DIR}/${vendor_dlkm_props_file}" ]]; then
      vendor_dlkm_props_file="${ROOT_DIR}/${vendor_dlkm_props_file}"
    elif [[ "${vendor_dlkm_props_file}" != /* ]]; then
      echo "ERROR: VENDOR_DLKM_PROPS must be an absolute path or relative to ${ROOT_DIR}: ${vendor_dlkm_props_file}" >&2
      exit 1
    elif [[ ! -f "${vendor_dlkm_props_file}" ]]; then
      echo "ERROR: Failed to find VENDOR_DLKM_PROPS: ${vendor_dlkm_props_file}"
      exit 1
    fi
  fi

  # Copy etc files to ${DIST_DIR} and ${VENDOR_DLKM_STAGING_DIR}/etc
  if [[ -n "${VENDOR_DLKM_ETC_FILES}" ]]; then
    local etc_files_dst_folder="${VENDOR_DLKM_STAGING_DIR}/etc"
    mkdir -p "${etc_files_dst_folder}"
    cp ${VENDOR_DLKM_ETC_FILES} "${etc_files_dst_folder}"
    cp ${VENDOR_DLKM_ETC_FILES} "${DIST_DIR}"
  fi

  build_image "${VENDOR_DLKM_STAGING_DIR}" "${vendor_dlkm_props_file}" \
    "${DIST_DIR}/vendor_dlkm.img" /dev/null

  if [ -z "${VENDOR_DLKM_IMAGE_NAME}" ]; then
    VENDOR_DLKM_IMAGE_NAME="vendor_dlkm.img"
  fi
  local generated_images=(${VENDOR_DLKM_IMAGE_NAME})

 # Build vendor_dlkm flatten image as /lib/modules/*.ko; if unset or null: default false
  if [[ ${VENDOR_DLKM_GEN_FLATTEN_IMAGE:-0} == "1" ]]; then
    local vendor_dlkm_flatten_image_name="vendor_dlkm.flatten.img"

    if [ -z "${VENDOR_DLKM_PROPS}" ]; then
      echo -e "fs_type=${VENDOR_DLKM_FS_TYPE}" >> ${vendor_dlkm_props_file}
      echo -e "mount_point=vendor_dlkm\n" >> ${vendor_dlkm_props_file}
    fi

  build_flattened_dlkm_image "${vendor_dlkm_flatten_image_name}" "${VENDOR_DLKM_STAGING_DIR}" \
    "${vendor_dlkm_props_file}" "${DIST_DIR}"

  generated_images+=(${vendor_dlkm_flatten_image_name})
  fi

  for image in "${generated_images[@]}"
  do
    avbtool add_hashtree_footer \
      --partition_name vendor_dlkm \
      --hash_algorithm sha256 \
      --image "${DIST_DIR}/${image}"
  done

  if [ -n "${vendor_dlkm_archive}" ]; then
    # Archive vendor_dlkm_staging_dir
    tar -czf "${DIST_DIR}/vendor_dlkm_staging_archive.tar.gz" -C "${VENDOR_DLKM_STAGING_DIR}" .
  fi
}

function build_super() {
  local super_props_file="${DIST_DIR}/super_image.props"
  local dynamic_partitions=""

  if [ -z "$SUPER_IMAGE_SIZE" ]; then
    echo "ERROR: SUPER_IMAGE_SIZE must be set" >&2
    exit 1
  fi
  local group_size="$((SUPER_IMAGE_SIZE - 0x400000))"
  cat << EOF >> "$super_props_file"
lpmake=lpmake
super_metadata_device=super
super_block_devices=super
super_super_device_size=${SUPER_IMAGE_SIZE}
super_partition_size=${SUPER_IMAGE_SIZE}
super_partition_groups=kb_dynamic_partitions
super_kb_dynamic_partitions_group_size=${group_size}
EOF

  if [[ -n "${SYSTEM_DLKM_IMAGE}" ]]; then
    echo -e "system_dlkm_image=${SYSTEM_DLKM_IMAGE}" >> "$super_props_file"
    dynamic_partitions="${dynamic_partitions} system_dlkm"
  fi
  if [[ -n "${VENDOR_DLKM_IMAGE}" ]]; then
    echo -e "vendor_dlkm_image=${VENDOR_DLKM_IMAGE}" >> "$super_props_file"
    dynamic_partitions="${dynamic_partitions} vendor_dlkm"
  fi

  echo -e "dynamic_partition_list=${dynamic_partitions}" >> "$super_props_file"
  echo -e "super_kb_dynamic_partitions_partition_list=${dynamic_partitions}" >> "$super_props_file"

  build_super_image -v "$super_props_file" "${DIST_DIR}/super.img"
  rm -f "$super_props_file"
}

function check_mkbootimg_path() {
  if [ -z "${MKBOOTIMG_PATH}" ]; then
    MKBOOTIMG_PATH="tools/mkbootimg/mkbootimg.py"
  fi
  if [ ! -f "${MKBOOTIMG_PATH}" ]; then
    echo "ERROR: mkbootimg.py script not found. MKBOOTIMG_PATH = ${MKBOOTIMG_PATH}" >&2
    exit 1
  fi
}

function build_boot_images() {
  check_mkbootimg_path

  BOOT_IMAGE_HEADER_VERSION=${BOOT_IMAGE_HEADER_VERSION:-3}
  MKBOOTIMG_ARGS=("--header_version" "${BOOT_IMAGE_HEADER_VERSION}")
  if [ -n  "${BASE_ADDRESS}" ]; then
    MKBOOTIMG_ARGS+=("--base" "${BASE_ADDRESS}")
  fi
  if [ -n  "${PAGE_SIZE}" ]; then
    MKBOOTIMG_ARGS+=("--pagesize" "${PAGE_SIZE}")
  fi
  if [ -n "${KERNEL_VENDOR_CMDLINE}" -a "${BOOT_IMAGE_HEADER_VERSION}" -lt "3" ]; then
    KERNEL_CMDLINE+=" ${KERNEL_VENDOR_CMDLINE}"
  fi
  if [ -n "${KERNEL_CMDLINE}" ]; then
    MKBOOTIMG_ARGS+=("--cmdline" "${KERNEL_CMDLINE}")
  fi
  # TODO: b/236012223 - [Kleaf] Migrate all build configs to BUILD.bazel
  #
  # These *_OFFSET variables should be migrated to be specified as attributes
  # for the kernel_images() macro.
  if [ -n "${TAGS_OFFSET}" ]; then
    MKBOOTIMG_ARGS+=("--tags_offset" "${TAGS_OFFSET}")
  fi
  if [ -n "${RAMDISK_OFFSET}" ]; then
    MKBOOTIMG_ARGS+=("--ramdisk_offset" "${RAMDISK_OFFSET}")
  fi
  if [ -n "${DTB_OFFSET}" ]; then
    MKBOOTIMG_ARGS+=("--dtb_offset" "${DTB_OFFSET}")
  fi
  if [ -n "${KERNEL_OFFSET}" ]; then
    MKBOOTIMG_ARGS+=("--kernel_offset" "${KERNEL_OFFSET}")
  fi

  DTB_FILE_LIST=$(find ${DIST_DIR} -name "*.dtb" | sort)
  if [ -z "${DTB_FILE_LIST}" ]; then
    if [ -z "${SKIP_VENDOR_BOOT}" ]; then
      echo "ERROR: No *.dtb files found in ${DIST_DIR}" >&2
      exit 1
    fi
  else
    cat $DTB_FILE_LIST > ${DIST_DIR}/dtb.img
    MKBOOTIMG_ARGS+=("--dtb" "${DIST_DIR}/dtb.img")
  fi

  rm -rf "${MKBOOTIMG_STAGING_DIR}"
  MKBOOTIMG_RAMDISK_STAGING_DIR="${MKBOOTIMG_STAGING_DIR}/ramdisk_root"
  mkdir -p "${MKBOOTIMG_RAMDISK_STAGING_DIR}"

  if [ -z "${SKIP_UNPACKING_RAMDISK}" ]; then
    if [ -n "${VENDOR_RAMDISK_BINARY}" ]; then
      VENDOR_RAMDISK_CPIO="${MKBOOTIMG_STAGING_DIR}/vendor_ramdisk_binary.cpio"
      rm -f "${VENDOR_RAMDISK_CPIO}"
      for vendor_ramdisk_binary in ${VENDOR_RAMDISK_BINARY}; do
        if ! [ -f "${vendor_ramdisk_binary}" ]; then
          echo "ERROR: Unable to locate vendor ramdisk ${vendor_ramdisk_binary}." >&2
          exit 1
        fi
        if ${DECOMPRESS_GZIP} "${vendor_ramdisk_binary}" 2>/dev/null >> "${VENDOR_RAMDISK_CPIO}"; then
          :
        elif ${DECOMPRESS_LZ4} "${vendor_ramdisk_binary}" 2>/dev/null >> "${VENDOR_RAMDISK_CPIO}"; then
          :
        elif cpio -t < "${vendor_ramdisk_binary}" &>/dev/null; then
          cat "${vendor_ramdisk_binary}" >> "${VENDOR_RAMDISK_CPIO}"
        else
          echo "ERROR: Unable to identify type of vendor ramdisk ${vendor_ramdisk_binary}" >&2
          rm -f "${VENDOR_RAMDISK_CPIO}"
          exit 1
        fi
      done

      # Remove lib/modules from the vendor ramdisk binary
      # Also execute ${VENDOR_RAMDISK_CMDS} for further modifications
      ( cd "${MKBOOTIMG_RAMDISK_STAGING_DIR}"
        cpio -idu --quiet <"${VENDOR_RAMDISK_CPIO}"
        rm -rf lib/modules
        eval ${VENDOR_RAMDISK_CMDS}
      )
    fi

  fi

  if [ -f "${VENDOR_FSTAB}" ]; then
    mkdir -p "${MKBOOTIMG_RAMDISK_STAGING_DIR}/first_stage_ramdisk"
    cp "${VENDOR_FSTAB}" "${MKBOOTIMG_RAMDISK_STAGING_DIR}/first_stage_ramdisk/"
  fi

  HAS_RAMDISK=
  MKBOOTIMG_RAMDISK_DIRS=()
  if [ -n "${VENDOR_RAMDISK_BINARY}" ] || [ -f "${VENDOR_FSTAB}" ]; then
    HAS_RAMDISK="1"
    MKBOOTIMG_RAMDISK_DIRS+=("${MKBOOTIMG_RAMDISK_STAGING_DIR}")
  fi

  if [ "${BUILD_INITRAMFS}" = "1" ]; then
    HAS_RAMDISK="1"
    if [ -z "${INITRAMFS_VENDOR_RAMDISK_FRAGMENT_NAME}" ]; then
      MKBOOTIMG_RAMDISK_DIRS+=("${INITRAMFS_STAGING_DIR}")
    fi
  fi

  if [ -z "${HAS_RAMDISK}" ] && [ -z "${SKIP_VENDOR_BOOT}" ]; then
    echo "ERROR: No ramdisk found. Please provide a GKI and/or a vendor ramdisk." >&2
    exit 1
  fi

  MKBOOTFS_ARGS=()
  if [ -n "${VENDOR_RAMDISK_DEV_NODES}" ]; then
    for vendor_ramdisk_dev_nodes in ${VENDOR_RAMDISK_DEV_NODES}; do
      MKBOOTFS_ARGS+=("-n" "${vendor_ramdisk_dev_nodes}")
    done
  fi

  if [ -n "${SKIP_UNPACKING_RAMDISK}" ] && [ -e "${VENDOR_RAMDISK_BINARY}" ]; then
    cp "${VENDOR_RAMDISK_BINARY}" "${DIST_DIR}/ramdisk.${RAMDISK_EXT}"
  elif [ "${#MKBOOTIMG_RAMDISK_DIRS[@]}" -gt 0 ] || [ "${#MKBOOTFS_ARGS[@]}" -gt 0 ]; then
    MKBOOTIMG_RAMDISK_CPIO="${MKBOOTIMG_STAGING_DIR}/ramdisk.cpio"
    mkbootfs "${MKBOOTIMG_RAMDISK_DIRS[@]}" "${MKBOOTFS_ARGS[@]}" >"${MKBOOTIMG_RAMDISK_CPIO}"
    ${RAMDISK_COMPRESS} "${MKBOOTIMG_RAMDISK_CPIO}" >"${DIST_DIR}/ramdisk.${RAMDISK_EXT}"
  fi

  if [ -n "${BUILD_BOOT_IMG}" ]; then
    if [ ! -f "${DIST_DIR}/$KERNEL_BINARY" ]; then
      echo "ERROR: kernel binary(KERNEL_BINARY = $KERNEL_BINARY) not present in ${DIST_DIR}" >&2
      exit 1
    fi
    MKBOOTIMG_ARGS+=("--kernel" "${DIST_DIR}/${KERNEL_BINARY}")
  fi

  if [ "${BOOT_IMAGE_HEADER_VERSION}" -ge "4" ] \
     && [ -n "${BUILD_INIT_BOOT_IMG}" ]; then
    INIT_BOOT_IMAGE_FILENAME="init_boot.img"
    MKINITBOOTIMG_ARGS+=("--output" "${DIST_DIR}/${INIT_BOOT_IMAGE_FILENAME}")
  fi

  if [ "${BOOT_IMAGE_HEADER_VERSION}" -ge "4" ]; then
    if [ -n "${VENDOR_BOOTCONFIG}" ]; then
      for PARAM in ${VENDOR_BOOTCONFIG}; do
        echo "${PARAM}"
      done >"${DIST_DIR}/vendor-bootconfig.img"
      MKBOOTIMG_ARGS+=("--vendor_bootconfig" "${DIST_DIR}/vendor-bootconfig.img")
      KERNEL_VENDOR_CMDLINE+=" bootconfig"
    fi
  fi

  if [ "${BOOT_IMAGE_HEADER_VERSION}" -ge "3" ]; then
    if [ -f "${GKI_RAMDISK_PREBUILT_BINARY}" ]; then
      if [ "${BOOT_IMAGE_HEADER_VERSION}" -ge "4" ] \
         && [ -n "${BUILD_INIT_BOOT_IMG}" ]; then
        MKINITBOOTIMG_ARGS+=("--ramdisk" "${GKI_RAMDISK_PREBUILT_BINARY}")
        MKINITBOOTIMG_ARGS+=("--header_version" "${BOOT_IMAGE_HEADER_VERSION}")
      else
        MKBOOTIMG_ARGS+=("--ramdisk" "${GKI_RAMDISK_PREBUILT_BINARY}")
      fi
    fi

    if [ "${BUILD_VENDOR_KERNEL_BOOT}" = "1" ]; then
      VENDOR_BOOT_NAME="vendor_kernel_boot.img"
    elif [ -z "${SKIP_VENDOR_BOOT}" ]; then
      VENDOR_BOOT_NAME="vendor_boot.img"
    fi
    if [ -n "${VENDOR_BOOT_NAME}" ]; then
      MKBOOTIMG_ARGS+=("--vendor_boot" "${DIST_DIR}/${VENDOR_BOOT_NAME}")
      if [ -n "${KERNEL_VENDOR_CMDLINE}" ]; then
        MKBOOTIMG_ARGS+=("--vendor_cmdline" "${KERNEL_VENDOR_CMDLINE}")
      fi
      if [ -f "${DIST_DIR}/ramdisk.${RAMDISK_EXT}" ]; then
        MKBOOTIMG_ARGS+=("--vendor_ramdisk" "${DIST_DIR}/ramdisk.${RAMDISK_EXT}")
      fi
      if [ "${BUILD_INITRAMFS}" = "1" ] \
          && [ -n "${INITRAMFS_VENDOR_RAMDISK_FRAGMENT_NAME}" ]; then
        MKBOOTIMG_ARGS+=("--ramdisk_type" "DLKM")
        for MKBOOTIMG_ARG in ${INITRAMFS_VENDOR_RAMDISK_FRAGMENT_MKBOOTIMG_ARGS}; do
          MKBOOTIMG_ARGS+=("${MKBOOTIMG_ARG}")
        done
        MKBOOTIMG_ARGS+=("--ramdisk_name" "${INITRAMFS_VENDOR_RAMDISK_FRAGMENT_NAME}")
        MKBOOTIMG_ARGS+=("--vendor_ramdisk_fragment" "${DIST_DIR}/initramfs.img")
      fi
    fi
  else
    if [ -f "${DIST_DIR}/ramdisk.${RAMDISK_EXT}" ]; then
      MKBOOTIMG_ARGS+=("--ramdisk" "${DIST_DIR}/ramdisk.${RAMDISK_EXT}")
    fi
  fi

  if [ -z "${BOOT_IMAGE_FILENAME}" ]; then
    BOOT_IMAGE_FILENAME="boot.img"
  fi
  if [ -n "${BUILD_BOOT_IMG}" ]; then
    MKBOOTIMG_ARGS+=("--output" "${DIST_DIR}/${BOOT_IMAGE_FILENAME}")
  fi

  for MKBOOTIMG_ARG in ${MKBOOTIMG_EXTRA_ARGS}; do
    MKBOOTIMG_ARGS+=("${MKBOOTIMG_ARG}")
  done

  "${MKBOOTIMG_PATH}" "${MKBOOTIMG_ARGS[@]}"

  if [ "${BOOT_IMAGE_HEADER_VERSION}" -ge "4" ] \
     && [ -n "${BUILD_INIT_BOOT_IMG}" ]; then
    "${MKBOOTIMG_PATH}" "${MKINITBOOTIMG_ARGS[@]}"
  fi

  if [ -n "${BUILD_INIT_BOOT_IMG}" ] \
      && [ -f "${DIST_DIR}/${INIT_BOOT_IMAGE_FILENAME}" ]; then
    echo "init_boot image created at ${DIST_DIR}/${INIT_BOOT_IMAGE_FILENAME}"
  fi

  if [ -n "${BUILD_BOOT_IMG}" -a -f "${DIST_DIR}/${BOOT_IMAGE_FILENAME}" ]; then
    if [ -n "${AVB_SIGN_BOOT_IMG}" ]; then
      if [ -n "${AVB_BOOT_PARTITION_SIZE}" ] \
          && [ -n "${AVB_BOOT_KEY}" ] \
          && [ -n "${AVB_BOOT_ALGORITHM}" ]; then
        if [ -z "${AVB_BOOT_PARTITION_NAME}" ]; then
          AVB_BOOT_PARTITION_NAME=${BOOT_IMAGE_FILENAME%%.*}
        fi

        avbtool add_hash_footer \
            --partition_name ${AVB_BOOT_PARTITION_NAME} \
            --partition_size ${AVB_BOOT_PARTITION_SIZE} \
            --image "${DIST_DIR}/${BOOT_IMAGE_FILENAME}" \
            --algorithm ${AVB_BOOT_ALGORITHM} \
            --key ${AVB_BOOT_KEY}
      else
        echo "ERROR: Missing the AVB_* flags. Failed to sign the boot image" 1>&2
        exit 1
      fi
    fi
  fi
}

function make_dtbo() {
  (
    cd ${OUT_DIR}
    mkdtimg create "${DIST_DIR}"/dtbo.img ${MKDTIMG_FLAGS} ${MKDTIMG_DTBOS}
  )
}

# gki_get_boot_img_size <compression method>.
# The function echoes the value of the preconfigured size variable
# based on the input compression method.
#   - (empty): echo ${BUILD_GKI_BOOT_IMG_SIZE}
#   -      gz: echo ${BUILD_GKI_BOOT_IMG_GZ_SIZE}
#   -     lz4: echo ${BUILD_GKI_BOOT_IMG_LZ4_SIZE}
function gki_get_boot_img_size() {
  local compression

  if [ -z "$1" ]; then
    boot_size_var="BUILD_GKI_BOOT_IMG_SIZE"
  else
    compression=$(echo "$1" | tr '[:lower:]' '[:upper:]')
    boot_size_var="BUILD_GKI_BOOT_IMG_${compression}_SIZE"
  fi

  if [ -z "${!boot_size_var}" ]; then
    echo "ERROR: ${boot_size_var} is not set." >&2
    exit 1
  fi

  echo "${!boot_size_var}"
}

# gki_add_avb_footer <image> <partition_size> <security_patch_level>
function gki_add_avb_footer() {
  local spl_date="$3"
  local additional_props=""
  if [ -n "${spl_date}" ]; then
    additional_props="--prop com.android.build.boot.security_patch:${spl_date}"
  fi

  avbtool add_hash_footer --image "$1" \
    --partition_name boot --partition_size "$2" \
    ${additional_props}
}

# gki_dry_run_certify_bootimg <boot_image> <gki_artifacts_info_file> <security_patch_level>
# The certify_bootimg script will be executed on a server over a GKI
# boot.img during the official certification process, which embeds
# a GKI certificate into the boot.img. The certificate is for Android
# VTS to verify that a GKI boot.img is authentic.
# Dry running the process here so we can catch related issues early.
function gki_dry_run_certify_bootimg() {
  local spl_date="$3"
  local additional_props=()
  if [ -n "${spl_date}" ]; then
    additional_props+=("--extra_footer_args" \
      "--prop com.android.build.boot.security_patch:${spl_date}")
  fi

  certify_bootimg --boot_img "$1" \
    --algorithm SHA256_RSA4096 \
    --key tools/mkbootimg/gki/testdata/testkey_rsa4096.pem \
    --gki_info "$2" \
    --output "$1" \
    "${additional_props[@]}"
}

# build_gki_artifacts_info <output_gki_artifacts_info_file>
function build_gki_artifacts_info() {
  local artifacts_info="certify_bootimg_extra_args=--prop ARCH:${ARCH} --prop BRANCH:${BRANCH}"

  if [ -n "${BUILD_NUMBER}" ]; then
    artifacts_info="${artifacts_info} --prop BUILD_NUMBER:${BUILD_NUMBER}"
  fi

  KERNEL_RELEASE="$(cat "${OUT_DIR}"/include/config/kernel.release)"
  artifacts_info="${artifacts_info} --prop KERNEL_RELEASE:${KERNEL_RELEASE}"

  echo "${artifacts_info}" > "$1"

  echo "kernel_release=${KERNEL_RELEASE}" >> "$1"
}

# build_gki_boot_images <uncompressed kernel path>.
# The function builds boot-*.img for kernel images
# with the prefix of <uncompressed kernel path>.
# It also generates a boot-img.tar.gz containing those
# boot-*.img files. The uncompressed kernel image should
# exist, e.g., ${DIST_DIR}/Image, while other compressed
# kernel images are optional, e.g., ${DIST_DIR}/Image.gz.
function build_gki_boot_images() {
  local uncompressed_kernel_path=$1

  if ! [ -f "${uncompressed_kernel_path}" ]; then
    echo "ERROR: '${uncompressed_kernel_path}' doesn't exist" >&2
    exit 1
  fi

  uncompressed_kernel_image="$(basename "${uncompressed_kernel_path}")"

  DEFAULT_MKBOOTIMG_ARGS=("--header_version" "4")
  if [ -n "${GKI_KERNEL_CMDLINE}" ]; then
    DEFAULT_MKBOOTIMG_ARGS+=("--cmdline" "${GKI_KERNEL_CMDLINE}")
  fi

  GKI_ARTIFACTS_INFO_FILE="${DIST_DIR}/gki-info.txt"
  build_gki_artifacts_info "${GKI_ARTIFACTS_INFO_FILE}"
  local images_to_pack=("$(basename "${GKI_ARTIFACTS_INFO_FILE}")")

  # Compressed kernel images, e.g., Image.gz, Image.lz4 have the same
  # prefix as the uncompressed kernel image, e.g., Image.
  for kernel_path in "${uncompressed_kernel_path}"*; do
    GKI_MKBOOTIMG_ARGS=("${DEFAULT_MKBOOTIMG_ARGS[@]}")
    GKI_MKBOOTIMG_ARGS+=("--kernel" "${kernel_path}")

    if [ "${kernel_path}" = "${uncompressed_kernel_path}" ]; then
        boot_image="boot.img"
    else
        kernel_image="$(basename "${kernel_path}")"
        compression="${kernel_image#"${uncompressed_kernel_image}".}"
        boot_image="boot-${compression}.img"
    fi

    boot_image_path="${DIST_DIR}/${boot_image}"
    GKI_MKBOOTIMG_ARGS+=("--output" "${boot_image_path}")
    "${MKBOOTIMG_PATH}" "${GKI_MKBOOTIMG_ARGS[@]}"

    if [[ -z "${BUILD_GKI_BOOT_SKIP_AVB}" ]]; then
      # Pick a SPL date far enough in the future so that you can flash
      # development GKI kernels on an unlocked device without wiping the
      # userdata. This is for development purposes only and should be
      # overwritten by the Android platform build to include an accurate SPL.
      # Note, the certified GKI release builds will not include the SPL
      # property.
      local spl_month=$((($(date +'%-m') + 3) % 12))
      local spl_year="$(date +'%Y')"
      if [ $((${spl_month} % 3)) -gt 0 ]; then
        # Round up to the next quarterly platform release (QPR) month
        spl_month=$((${spl_month} + 3 - (${spl_month} % 3)))
      fi
      if [ "${spl_month}" -lt "$(date +'%-m')" ]; then
        # rollover to the next year
        spl_year="$((${spl_year} + 1))"
      fi
      local spl_date=$(printf "%d-%02d-05\n" ${spl_year} ${spl_month})

      gki_add_avb_footer "${boot_image_path}" \
        "$(gki_get_boot_img_size "${compression}")" "${spl_date}"
      gki_dry_run_certify_bootimg "${boot_image_path}" \
        "${GKI_ARTIFACTS_INFO_FILE}" "${spl_date}"
    fi
    images_to_pack+=("${boot_image}")
  done

  GKI_BOOT_IMG_ARCHIVE="boot-img.tar.gz"
  tar -czf "${DIST_DIR}/${GKI_BOOT_IMG_ARCHIVE}" -C "${DIST_DIR}" \
    "${images_to_pack[@]}"
}

function build_gki_artifacts() {
  check_mkbootimg_path

  if [ "${ARCH}" = "arm64" -o "${ARCH}" = "riscv64" ]; then
    build_gki_boot_images "${DIST_DIR}/Image"
  elif [ "${ARCH}" = "x86_64" ]; then
    build_gki_boot_images "${DIST_DIR}/bzImage"
  else
    echo "ERROR: unknown ARCH to BUILD_GKI_ARTIFACTS: '${ARCH}'" >&2
    exit 1
  fi
}

function sort_config() {
  # Normal sort won't work because all the "# CONFIG_.. is not set" would come
  # before all the "CONFIG_..=m". Use sed to extract the CONFIG_ option and prefix
  # the line in front of the line to create a key (e.g. CONFIG_.. # CONFIG_.. is not set),
  # sort, then remove the key
  sed -E -e 's/.*(CONFIG_[^ =]+).*/\1 \0/' $1 | sort -k1 | cut -F2-
}

function menuconfig() {
  set +x
  local orig_config=$(mktemp)
  local new_config="${OUT_DIR}/.config"
  local changed_config=$(mktemp)
  local new_fragment=$(mktemp)

  trap "rm -f ${orig_config} ${changed_config} ${new_fragment}" EXIT

  if [ -n "${FRAGMENT_CONFIG}" ]; then
    if [[ -f "${ROOT_DIR}/${FRAGMENT_CONFIG}" ]]; then
      FRAGMENT_CONFIG="${ROOT_DIR}/${FRAGMENT_CONFIG}"
    elif [[ "${FRAGMENT_CONFIG}" != /* ]]; then
      echo "ERROR: FRAGMENT_CONFIG must be an absolute path or relative to ${ROOT_DIR}: ${FRAGMENT_CONFIG}" >&2
      exit 1
    elif [[ ! -f "${FRAGMENT_CONFIG}" ]]; then
      echo "ERROR: Failed to find FRAGMENT_CONFIG: ${FRAGMENT_CONFIG}" >&2
      exit 1
    fi
  fi

  cp ${OUT_DIR}/.config ${orig_config}
  (cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} ${MAKE_ARGS} ${1:-menuconfig})

  if [ -z "${FRAGMENT_CONFIG}" ]; then
    (cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} ${MAKE_ARGS} savedefconfig)
    [ "$ARCH" = "x86_64" -o "$ARCH" = "i386" ] && local ARCH=x86
    echo "Updating $(realpath ${ROOT_DIR}/${KERNEL_DIR}/arch/${ARCH}/configs/${DEFCONFIG})"
    mv ${OUT_DIR}/defconfig $(realpath ${ROOT_DIR}/${KERNEL_DIR}/arch/${ARCH}/configs/${DEFCONFIG})
    return
  fi

  ${KERNEL_DIR}/scripts/diffconfig -m ${orig_config} ${new_config} > ${changed_config}
  KCONFIG_CONFIG=${new_fragment} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m ${FRAGMENT_CONFIG} ${changed_config}
  sort_config ${new_fragment} > $(realpath ${FRAGMENT_CONFIG})
  set +x


  echo
  echo "Updated $(realpath ${FRAGMENT_CONFIG})"
  echo
}

# $1: A mapping of the form path:value [path:value [...]]
# $2: A path. This may be a subpath of an item in the mapping
# $3: What is being determined (for error messages)
# Returns the corresponding value of path.
# Example:
#   extract_git_metadata "foo:123 bar:456" foo/baz
#   -> 123
function extract_git_metadata() {
  local map=$1
  local git_project_candidate=$2
  local what=$3
  while [[ "${git_project_candidate}" != "." ]]; do
    value_candidate=$(python3 -c '
import sys, json
js = json.load(sys.stdin)
key = sys.argv[1]
if key in js:
    print(js[key])
' "${git_project_candidate}" <<< "${map}")
    if [[ -n "${value_candidate}" ]]; then
        break
    fi
    git_project_candidate=$(dirname ${git_project_candidate})
  done
  if [[ -n ${value_candidate} ]]; then
    echo "${value_candidate}"
  else
    echo "WARNING: Can't determine $what for $2" >&2
  fi
}

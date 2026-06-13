/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

 String related routines

 GENERAL DESCRIPTION
 This component implements portable string related routines

 Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-3-Clause-Clear
 =============================================================================*/

#ifndef __QC_LOC_FW_STRING_H__
#define __QC_LOC_FW_STRING_H__

#include <base_util/log.h>

namespace qc_loc_fw
{

#ifdef IZAT_OFFLINE
// Android has its own strlcpy
size_t strlcpy(char *dest, const char *src, size_t dest_capacity);
// Android has its own strlcat
size_t strlcat(char *dest, const char *src, size_t dest_capacity);
#endif // #ifdef IZAT_OFFLINE

} // namespace qc_loc_fw

#endif //#ifndef __QC_LOC_FW_STRING_H__

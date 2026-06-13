/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

 String related routines

 GENERAL DESCRIPTION
 This component provides utilities used for string processing

 Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-3-Clause-Clear
 =============================================================================*/
#include <string.h>

#include <base_util/log.h>

#ifndef IZAT_OFFLINE
#define HAS_STRLCPY 1
#endif // #ifndef IZAT_OFFLINE

#define BREAK_IF_ZERO(ERR,X) if(0==(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO(ERR,X) if(0!=(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO_RC(ERR,RC,X) if(0!=(RC=(X))) {result = (ERR); break;}

namespace qc_loc_fw
{

#if HAS_STRLCPY
#else // #if HAS_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t dest_capacity)
{
  // strlcpy is not available on standard linux glibc, so I have to
  // implement this
  // somehow Android has its own strlcpy
  size_t src_length = strlen(src);
  if(dest_capacity > 0)
  {
    size_t copy_length = 0;
    if(src_length >= dest_capacity)
    {
      copy_length = dest_capacity - 1;
    }
    else
    {
      copy_length = src_length;
    }
    memcpy(dest, src, copy_length);
    dest[copy_length] = '\0';
  }
  else
  {
    // weird error
    src_length = 0;
  }
  return src_length;
}

size_t strlcat(char *dest, const char *src, size_t dest_capacity)
{
  size_t dest_length = strlen(dest);
  size_t src_length = strlen(src);
  if (dest_capacity > 0)
  {
    size_t copy_length = 0;
    if (dest_length + src_length >= dest_capacity)
    {
      copy_length = dest_capacity - dest_length - 1;
    }
    else
    {
      copy_length = src_length;
    }
    memcpy(dest + dest_length, src, copy_length);
    dest_length += copy_length;
    dest[dest_length] = '\0';
  }
  else
  {
    // weird error
    dest_length = 0;
  }
  return dest_length;
}
#endif // #if HAS_STRLCPY

} // namespace qc_loc_fw


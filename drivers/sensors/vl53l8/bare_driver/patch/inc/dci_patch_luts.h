/*******************************************************************************
* Copyright (c) 2022, STMicroelectronics - All Rights Reserved
*
* This file is part of VL53L8 Kernel Driver and is dual licensed,
* either 'STMicroelectronics Proprietary license'
* or 'BSD 3-clause "New" or "Revised" License' , at your option.
*
********************************************************************************
*
* 'STMicroelectronics Proprietary license'
*
********************************************************************************
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0081
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*
********************************************************************************
*
* Alternatively, VL53L8 Kernel Driver may be distributed under the terms of
* 'BSD 3-clause "New" or "Revised" License', in which case the following
* provisions apply instead of the ones mentioned above :
*
********************************************************************************
*
* License terms: BSD 3-clause "New" or "Revised" License.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
*******************************************************************************/

#ifndef __DCI_PATCH_LUTS_H__
#define __DCI_PATCH_LUTS_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DCI_D16_BH_PARM_ID__END_OF_DATA \
	((uint32_t) 0U)

#define DCI_D16_BH_PARM_ID__EMPTY \
	((uint32_t) 1U)

#define DCI_D16_BH_PARM_ID__PACKET_HEADER \
	((uint32_t) 2U)

#define DCI_D16_BH_PARM_ID__RNG_META_DATA \
	((uint32_t) 3U)

#define DCI_D16_BH_PARM_ID__RNG_COMMON_DATA \
	((uint32_t) 4U)

#define DCI_D16_BH_PARM_ID__RNG_TIMING_DATA \
	((uint32_t) 5U)

#define DCI_D16_BH_PARM_ID__GLASS_DETECT \
	((uint32_t) 6U)

#define DCI_D16_BH_PARM_ID__DEPTH16 \
	((uint32_t) 7U)

#define DCI_D16_BH_PARM_ID__AMB_RATE_KCPS_PER_SPAD \
	((uint32_t) 8U)

#define DCI_D16_BH_PARM_ID__PEAK_RATE_KCPS_PER_SPAD \
	((uint32_t) 9U)

#define DCI_D16_BH_PARM_ID__TARGET_STATUS \
	((uint32_t) 10U)

#define DCI_D16_BH_PARM_ID__SZ__DEPTH16 \
	((uint32_t) 11U)

#define DCI_D16_BH_PARM_ID__SZ__AMB_RATE_KCPS_PER_SPAD \
	((uint32_t) 12U)

#define DCI_D16_BH_PARM_ID__SZ__PEAK_RATE_KCPS_PER_SPAD \
	((uint32_t) 13U)

#define DCI_D16_BH_PARM_ID__SZ__TARGET_STATUS \
	((uint32_t) 14U)

#define DCI_TARGET_STATUS__GLARE \
	((uint8_t) 18U)

#ifdef __cplusplus
}
#endif

#endif

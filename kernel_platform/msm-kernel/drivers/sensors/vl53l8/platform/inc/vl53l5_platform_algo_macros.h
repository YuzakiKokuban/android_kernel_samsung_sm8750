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

#ifndef VL53L5_PLATFORM_ALGO_MACROS_H
#define VL53L5_PLATFORM_ALGO_MACROS_H


#include <linux/math64.h>
#include <linux/kernel.h>
#include "vl53l5_platform_maths.h"


#ifdef __cplusplus
extern "C"
{
#endif


/** @defgroup VL53L5_PLATFORM_ALGOS_MACROS module
 * Platform algo macro definitions for math functions
 * @{
 */


/**
 * @def DIV_64D64_64
 * @brief customer supplied division operation - 64-bit unsigned
 *
 * @param dividend      unsigned 64-bit numerator
 * @param divisor       unsigned 64-bit denominator
 *
 * @return     res : 64-bit result
 */

#define DIV_64D64_64(dividend, divisor) div64_u64(dividend, divisor)

/**
 * @def DIVS_64D64_64
 * @brief customer supplied division operation - 64-bit signed
 *
 * @param dividend      signed 64-bit numerator
 * @param divisor       signed 64-bit denominator
 *
 * @return     res : 64-bit result
 */
#define DIVS_64D64_64(dividend, divisor) div64_s64(dividend, divisor)

/**
 * @brief Calculates the 64-bit result for the multiplication of two 32-bit
 *        unsigned numbers
 *
 * @param[in]  a0 : input unsigned 32-bit integer
 * @param[in]  a1 : input unsigned 32-bit integer
 *
 * @return     res : unsigned 64-bit result
 */
#define MUL_32X32_64(a0, a1) mul_u64_u64_shr((uint64_t)a0, (uint64_t)a1, 0)

/**
 * @brief Calculates the 64-bit result for the multiplication of two 32-bit
 *        signed numbers
 *
 * @param[in]  a0 : input signed 32-bit integer
 * @param[in]  a1 : input signed 32-bit integer
 *
 * @return     res : signed 64-bit result
 */

#define MULS_32X32_64(a0, a1) (int64_t)((int64_t)a0 * (int64_t)a1)


/**
 * @brief Calculates the square root of the input integer
 *
 * Reference : http://en.wikipedia.org/wiki/Methods_of_computing_square_roots
 *
 * @param[in]  num : input unsigned 32-bit integer
 *
 * @return     res : square root result - unsigned 32-bit integer
 */
#define INTEGER_SQRT(num) int_sqrt(num)

/**
 * @brief Returns the number of leading zero bits in the input unsigned 32-bit integer,
 *        starting from bit 31 down to bit 0
 *
 * @param[in]  num : input unsigned 32-bit integer
 *
 * @return     res : number of leading zero bits - signed 32-bit integer
 */

#define LEADING_ZERO_COUNT(num) __builtin_clz(num)


#ifdef __cplusplus
}
#endif

/** @} */

#endif /* VL53L5_PLATFORM_ALGO_MACROS_H */

/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

@VintfStability
@Backing(type="int")
enum AgmMediaFormat {
        AGM_FORMAT_INVALID,
        AGM_FORMAT_PCM_S8,          /**< 8-bit signed */
        AGM_FORMAT_PCM_S16_LE,      /**< 16-bit signed */
        AGM_FORMAT_PCM_S24_LE,      /**< 24-bits in 4-bytes */
        AGM_FORMAT_PCM_S24_3LE,     /**< 24-bits in 3-bytes */
        AGM_FORMAT_PCM_S32_LE,      /**< 32-bit signed */
        AGM_FORMAT_MP3,             /**< MP3 codec */
        AGM_FORMAT_AAC,             /**< AAC codec */
        AGM_FORMAT_FLAC,            /**< FLAC codec */
        AGM_FORMAT_ALAC,            /**< ALAC codec */
        AGM_FORMAT_APE,             /**< APE codec */
        AGM_FORMAT_WMASTD,          /**< WMA codec */
        AGM_FORMAT_WMAPRO,          /**< WMA pro codec */
        AGM_FORMAT_VORBIS,          /**< Vorbis codec */
        AGM_FORMAT_AMR_NB,          /**< AMR NB codec */
        AGM_FORMAT_AMR_WB,          /**< AMR WB codec */
        AGM_FORMAT_AMR_WB_PLUS,     /**< AMR WB Plus codec */
        AGM_FORMAT_EVRC,            /**< EVRC codec */
        AGM_FORMAT_G711,            /**< G711 codec */
        AGM_FORMAT_QCELP,            /**< G711 codec */
        AGM_FORMAT_MAX,
}

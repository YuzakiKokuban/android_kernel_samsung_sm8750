/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file CacV2Config.aidl
 * @brief Struct for the CAC V2 configurations
 *
 * This structure holds the configuration parameters for the CAC V2 algorithm.
 */
package vendor.qti.hardware.display.config;

@VintfStability
/**
 * @struct CacV2Config
 */
parcelable CacV2Config {
    /**
     * @brief Red color center position phase step
     */
    double k0r;

    /**
     * @brief Red color second-order phase step
     */
    double k1r;

    /**
     * @brief Blue color center position phase step
     */
    double k0b;

    /**
     * @brief Blue color second-order phase step
     */
    double k1b;

    /**
     * @brief Pixel pitch is device dependent
     */
    double pixel_pitch;

    /**
     * @brief Normalization factor
     */
    double normalization;
}

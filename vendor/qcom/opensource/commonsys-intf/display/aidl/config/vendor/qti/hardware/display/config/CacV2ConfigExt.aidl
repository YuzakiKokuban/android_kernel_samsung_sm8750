/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file CacV2ConfigExt.aidl
 * @brief Struct for the CAC V2 extended configurations
 *
 * This structure holds the extended configuration parameters for the CAC V2 algorithm.
 */
package vendor.qti.hardware.display.config;

@VintfStability
/**
 * @struct CacV2ConfigExt
 */
parcelable CacV2ConfigExt {
    /**
     * @brief Red color's phase step for center position
     */
    double redCenterPhaseStep;

    /**
     * @brief Red color's second order phase step
     */
    double redSecondOrderPhaseStep;

    /**
     * @brief Blue color's phase step for center position
     */
    double blueCenterPhaseStep;

    /**
     * @brief Blue color's second-order phase step
     */
    double blueSecondOrderPhaseStep;

    /**
     * @brief Pixel pitch is device dependent
     */
    double pixelPitch;

    /**
     * @brief Normalization factor
     */
    double normalization;

    /**
     * @brief Lens' vertical center position with respect to display panel. If 0, treated as
     * (height/2 - 1) by display service
     */
    int verticalCenter;

    /**
     * @brief Lens' horizontal center position with respect to display panel. If 0, treated as
     * (width/2 - 1) by display service
     */
    int horizontalCenter;
}

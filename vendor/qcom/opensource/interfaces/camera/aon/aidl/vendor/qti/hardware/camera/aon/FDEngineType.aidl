/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

/**
 * The underlying engine type of the algorithm
 */
@VintfStability
@Backing(type="int")
enum FDEngineType {
    /**
     * CADL engine (Lower power, lower memory usage engine).
     */
    EngineCADL = 0,
    /**
     * eNPU engine (Higher power, higher memory usage engine).
     */
    EngineENPU = 1,
}

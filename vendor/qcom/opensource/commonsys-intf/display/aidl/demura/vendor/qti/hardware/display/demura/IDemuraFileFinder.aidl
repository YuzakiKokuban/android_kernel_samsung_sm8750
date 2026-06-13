/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.display.demura;

@VintfStability
interface IDemuraFileFinder {
    @VintfStability
    parcelable DemuraFilePaths {
        String configFilePath;
        String signatureFilePath;
        String publickeyFilePath;
    }

    /**
     * Get file paths of demura files
     * @param panel_id id of panel
     * @param out file_paths paths of demura files
     */
    void getDemuraFilePaths(in long panel_id, out DemuraFilePaths file_paths);
}

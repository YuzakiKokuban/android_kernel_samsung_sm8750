/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 package vendor.qti.hardware.display.aiqe;

 @VintfStability
interface IDisplayAiqe {
    /**
     * Set the active SSRC mode for the specified display.
     * The SSRC feature must be active for this to have an effect
     *
     * @param dispId ID of the display to target.
     * @param mode_name Name of the SSRC mode to set.
     * @return OK on success or error if any parameters are invalid.
     */
    void setSsrcMode(in int disp_id, in String mode_name);
    /**
     * Enable COPR feature for the specified display.
     *
     * @param dispId ID of the display to target.
     * @param enable enable/disable COPR feature
     * @return OK on success or error if any parameters are invalid.
     */
    void enableCopr(in int disp_id, in boolean enable);
    /**
     * Query the COPR statistics for the specified display.
     * COPR feature must be active for this to work
     *
     * @param dispId ID of the display to target.
     * @return vector of COPR statistic data
     */
    int[] getCoprStats(in int disp_id);
    /**
     * Set ABC feature State
     *
     * @param dispId ID of the display to target.
     * @param enable control ABC feature enable/disable
     * @return error is NONE upon success
     */
    void setABCState(in int dispId, in int enable);
    /**
     * Set ABC feature Reconfig
     *
     * @param dispId ID of the display to target.
     * @return error is NONE upon success
     */
    void setABCReconfig(in int dispId);
    /**
     * Set ABC feature mode
     *
     * @param dispId ID of the display to target.
     * @param mode name of the ABC feature
     * @return error is NONE upon success
     */
    void setABCMode(in int dispId, in String mode_name);

    /**
     * Set the Mdnie mode.
     *
     * @param mode_name Name of the Mdnie Mode to set.
     * @return OK on success or error if any parameters are invalid.
     */
    void setMdnieMode(in String mode_name);
}

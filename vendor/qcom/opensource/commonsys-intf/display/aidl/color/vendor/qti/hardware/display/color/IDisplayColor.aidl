/*
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.display.color;

import vendor.qti.hardware.display.color.Result;
import vendor.qti.hardware.display.color.SprModeInfo;
import vendor.qti.hardware.display.color.PAConfig;
import vendor.qti.hardware.display.color.PARange;
import vendor.qti.hardware.display.color.PAEnable;
import vendor.qti.hardware.display.color.DisplayInfo;
import vendor.qti.hardware.display.color.DisplayNumInfo;

@VintfStability
interface IDisplayColor {
    /**
     * Initialize a color context.
     *
     * Each client is expected to call init interface and acquire a context
     * handle before exercising any other interface.
     *
     * @param flags client identifier.
     * @return context handle on success or negative value if anything wrong.
     */
    int init(in int flags);

    /**
     * De-initialize a color context.
     *
     * Client must free the context after use.
     *
     * @param  ctxHandle context handle.
     * @param  flags reserved.
     * @return OK on success or BAD_VALUE if any parameters are invalid.
     */
    Result deInit(in int ctxHandle, in int flags);

    /**
     * Initialize the qdcm socket service.
     *
     * Client needs to call this function to start/stop the socket service
     */
    void toggleSocketService(in boolean enable);

    /**
     * Get render intents map.
     *
     * Clients can get the render intents string and enums map for display specified by id.
     *
     * @param  disp_id is display ID.
     * @param out render_intent_string is string vector for all the render intents.
     * @param out render_intent_enum is numbers for all the render intents.
     * @return OK on success or BAD_VALUE if any parameters are invalid.
     */
    Result getRenderIntentsMap(in int disp_id,
        out String[] render_intent_string, out int[] render_intent_enum);

    /**
     * Get spr mode configuration.
     *
     * @param  ctxHandle context handle.
     * @param  dispId display id.
     * @param out info spr mode configuration.
     * @return OK on success or BAD_VALUE if any parameters are invalid.
     */
    Result getSPRMode(in int ctxHandle, in int dispId, out SprModeInfo info);

    /**
     * Set spr mode.
     *
     * @param  ctxHandle context handle.
     * @param  dispId display id.
     * @param  cfg spr mode configuration.
     * @return OK on success or error if any parameters are invalid.
     */
    Result setSPRMode(in int ctxHandle, in int dispId, in SprModeInfo info);

    /**
     * Query number of available displays.
     *
     * Clients can query number of various displays supported i.e.
     *   - Primary display
     *   - External display (if supported)
     *
     * @param  ctxHandle context handle.
     * @param out dispNumInfo number of supported displays and flags reserved
     * @return OK on success or error if any parameters are invalid.
     */
    Result getNumDisplay(in long ctxHandle, out DisplayNumInfo dispNumInfo);

    /**
     * Query IDs of available displays.
     *
     * Clients can query IDs of all the displays.
     *
     * @param  ctxHandle context handle.
     * @param out display_id list of display IDs.
     * @return OK on success or error if any parameters are invalid.
     */
    Result getDisplayId(in long ctxHandle, out long[] display_id);

    /**
     * Enumarate a requested display.
     *
     * Enumarates client requested display identified by the index.
     *
     * @param  ctxHandle context handle.
     * @param  index display index.
     * @param out dispInfo display information @DisplayInfo.
     * @return OK on success or error if any parameters are invalid.
     */
    Result getDisplay(in long ctxHandle, in int index, out DisplayInfo dispInfo);

    /**
     * Get supported global picture adjustment range.
     *
     * Gets supported picture adjustment range for hue, saturation, value and
     * contrast.
     *
     * @param  ctxHandle context handle.
     * @param  dispId display id.
     * @param out range supported picture adjustment range.
     * @return OK on success or error if any parameters are invalid.
     */
    Result getGlobalPARange(in long ctxHandle, in int dispId, out PARange range);

    /**
     * Get global picture adjustment configuration.
     *
     * @param  ctxHandle context handle.
     * @param  dispId display id.
     * @param out paEnable picture adjustment enabled on hw or cache.
     * @param out cfg user applied specific picture adjustment coefficients.
     * @return OK on success or error if any parameters are invalid.
     */
    Result getGlobalPA(in long ctxHandle, in int dispId, out PAEnable paEnable, out PAConfig cfg);

    /**
     * Set global picture adjustment configuration.
     *
     * @param  ctxHandle context handle.
     * @param  dispId display id.
     * @param  enable enables picture adjustment on hw or cache.
     * @param  cfg user specific picture adjustment coefficients.
     * @return OK on success or BAD_VALUE if any parameters are invalid.
     */
    Result setGlobalPA(in long ctxHandle, in int dispId, in int enable, in PAConfig cfg);
}

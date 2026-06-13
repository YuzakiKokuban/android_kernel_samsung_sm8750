/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.display.composer3;
import android.hardware.graphics.composer3.ClientTarget;
import vendor.qti.hardware.display.composer3.QtiLayerCommand;


@VintfStability
parcelable QtiDisplayCommand {
    /**
     * The display which this commands refers to.
     */
    long display;

    /**
     * Sets Qti layer commands for this display.
     * @see QtiLayerCommand.
     */
    QtiLayerCommand[] qtiLayers;

    /**
     * Extension of ClientTarget API.
     * @see ClientTarget.
     */
    @nullable ClientTarget clientTarget_3_1;

    /**
     * The display elapse time.
     */
    long time;
}

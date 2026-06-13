/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.display.composer3;
import vendor.qti.hardware.display.composer3.QtiLayerType;
import vendor.qti.hardware.display.composer3.QtiLayerFlags;

@VintfStability
parcelable QtiLayerCommand {
    /**
     * The layer which this commands refers to.
     * @see IComposer.createLayer
     */
    long layer;

    /**
    * Type of layer - this is a hint to HWC to handle the layer appropriately.
    */
    QtiLayerType qtiLayerType;

    /**
    * Flags attached to the layer
    */
    QtiLayerFlags qtiLayerFlags;

}

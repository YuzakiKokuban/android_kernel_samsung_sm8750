/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.display.postproc;

@VintfStability
interface IDisplayPostproc {
    /**
     * Send command to DPPS.
     * @param cmd string command to control post processing features.
     * @return response string response for the input dpps command.
     */
    String sendDPPSCommand(in String cmd);
}

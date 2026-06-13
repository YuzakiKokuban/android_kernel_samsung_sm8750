/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *            notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in the
 *            documentation and/or other materials provided with the distribution.
 *        * Neither the name of The Linux Foundation nor
 *            the names of its contributors may be used to endorse or promote
 *            products derived from this software without specific prior written
 *            permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package qcom.fmradio;

import java.util.List;

public class SpurDetails {

    private int RotationValue;
    private byte LsbOfIntegrationLength;
    private byte FilterCoefficeint;
    private byte IsEnableSpur;
    private byte SpurLevel;


    public int getRotationValue() {
        return RotationValue;
    }

    public void setRotationValue(int rotationValue) {
        RotationValue = rotationValue;
    }

    public byte getLsbOfIntegrationLength() {
        return LsbOfIntegrationLength;
    }

    public void setLsbOfIntegrationLength(byte lsbOfIntegrationLength) {
        LsbOfIntegrationLength = lsbOfIntegrationLength;
    }

    public byte getFilterCoefficeint() {
        return FilterCoefficeint;
    }

    public void setFilterCoefficeint(byte filterCoefficeint) {
        FilterCoefficeint = filterCoefficeint;
    }

    public byte getIsEnableSpur() {
        return IsEnableSpur;
    }

    public void setIsEnableSpur(byte isEnableSpur) {
        IsEnableSpur = isEnableSpur;
    }

    public byte getSpurLevel() {
        return SpurLevel;
    }

    public void setSpurLevel(byte spurLevel) {
        SpurLevel = spurLevel;
    }

    SpurDetails(int RotationValue, byte LsbOfIntegrationLength,
                byte FilterCoefficeint, byte IsEnableSpur, byte SpurLevel) {

        this.RotationValue = RotationValue;
        this.LsbOfIntegrationLength = LsbOfIntegrationLength;
        this.IsEnableSpur = IsEnableSpur;
        this.SpurLevel = SpurLevel;
    }

    public SpurDetails() {

    }

    public SpurDetails(SpurDetails spurDetails) {
        if (spurDetails != null) {
            this.RotationValue = spurDetails.RotationValue;
            this.LsbOfIntegrationLength =
                          spurDetails.LsbOfIntegrationLength;
            this.IsEnableSpur = spurDetails.IsEnableSpur;
            this.SpurLevel = spurDetails.SpurLevel;
        }
    }

}

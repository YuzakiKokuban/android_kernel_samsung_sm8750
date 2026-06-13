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

import java.util.regex.Pattern;

public final class SpurFileFormatConst {
    public static final String SPUR_MODE = "Mode";
    public static final String SPUR_NUM_ENTRY = "SpurNumEntry";
    public static final String SPUR_FREQ = "SpurFreq";
    public static final String SPUR_NO_OF = "NoOfSpursToTrack";
    public static final String SPUR_ROTATION_VALUE = "RotationValue";
    public static final String SPUR_LSB_LENGTH = "LsbOfIntegrationLength";
    public static final String SPUR_FILTER_COEFF = "FilterCoefficeint";
    public static final String SPUR_IS_ENABLE = "IsEnableSpur";
    public static final String SPUR_LEVEL = "SpurLevel";
    public static final char COMMENT = '#';
    public static final char DELIMETER = '=';
    public static final Pattern SPACE_PATTERN = Pattern.compile("\\s");
    public static int SPUR_DETAILS_FOR_EACH_FREQ_CNT = 5;

    public enum LineType {
        EMPTY_LINE,
        SPUR_MODE_LINE,
        SPUR_N_ENTRY_LINE,
        SPUR_FR_LINE,
        SPUR_NO_OF_LINE,
        SPUR_ROT0_LINE,
        SPUR_LSB0_LINE,
        SPUR_FILTER0_LINE,
        SPUR_ENABLE0_LINE,
        SPUR_LEVEL0_LINE,
    }
}

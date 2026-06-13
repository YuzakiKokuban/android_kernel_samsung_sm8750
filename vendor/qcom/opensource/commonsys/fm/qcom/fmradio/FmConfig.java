/*
 * Copyright (c) 2009-2011, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of The Linux Foundation nor
 *      the names of its contributors may be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package qcom.fmradio;

import android.util.Log;
import android.os.SystemProperties;
import java.util.List;

/**
 *
 * Class to be used when changing radio settings
 * @hide
 */
public class FmConfig {

    /* V4l2 Controls */
     private static final int V4L2_CID_PRIVATE_BASE                   = 0x8000000;
     private static final int V4L2_CID_PRIVATE_TAVARUA_REGION         = V4L2_CID_PRIVATE_BASE + 7;
     private static final int V4L2_CID_PRIVATE_TAVARUA_EMPHASIS       = V4L2_CID_PRIVATE_BASE + 12;
     private static final int V4L2_CID_PRIVATE_TAVARUA_RDS_STD        = V4L2_CID_PRIVATE_BASE + 13;
     private static final int V4L2_CID_PRIVATE_TAVARUA_SPACING        = V4L2_CID_PRIVATE_BASE + 14;
     private static final int V4L2_CID_PRIVATE_TAVARUA_SRCH_ALGORITHM = V4L2_CID_PRIVATE_BASE + 0x2B;

     private static final int each_Spur_entry_size = 16;
     public static final int no_Of_Spurs_For_Entry = 3;

     private static final String TAG = "FmConfig";



    private int mRadioBand;
    /**
     * FM pre-emphasis/de-emphasis
     *
     * Possible Values:
     *
     * FmTransceiver.FM_DE_EMP75,
     * FmTransceiver.FM_DE_EMP50
     */
    private int mEmphasis;
    /**
     * Channel spacing
     *
     * Possible Values:
     *
     * FmTransceiver.FM_CHSPACE_200_KHZ,
     * FmTransceiver.FM_CHSPACE_100_KHZ,
     * FmTransceiver.FM_CHSPACE_50_KHZ
     */
    private int mChSpacing;
    /**
     * RDS standard type
     *
     * Possible Values:
     *
     * FmTransceiver.FM_RDS_STD_RBDS,
     * FmTransceiver.FM_RDS_STD_RDS,
     * FmTransceiver.FM_RDS_STD_NONE
     */
    private int mRdsStd;

    /**
     * FM Frequency Band Lower Limit in KHz
     */
    private int mBandLowerLimit;
    /**
     * FM Frequency Band Upper Limit in KHz
     */
    private int mBandUpperLimit;

    public static boolean fmSpurConfig(int fd) {
        SpurFileParser parser = new SpurFileParser();
        SpurTable t;
        int freq = 0, i = 0, j = 0;
        byte no_of_spur_freq = 0;
        int rotation_value = 0;
        int re;

        t = parser.GetSpurTable("/etc/fm/SpurTableFile.txt");
        List <Spur> list = t.GetSpurList();
        no_of_spur_freq = t.GetspurNoOfFreq();
        short [] buff = new short[2 + (no_of_spur_freq * each_Spur_entry_size)];
        buff[0] = t.GetMode();
        buff[1] = no_of_spur_freq;
        for(i = 0; i < no_of_spur_freq; i++) {
            List <Spur> spur = t.GetSpurList();
            freq = spur.get(i).getSpurFreq();
            buff[(i * each_Spur_entry_size) + 2] = (short)(freq & 0xff);
            buff[(i * each_Spur_entry_size) + 3] = (short)((freq >> 8) & 0xff);
            buff[(i * each_Spur_entry_size) + 4] = (short)((freq >> 16) & 0xff);
            buff[(i * each_Spur_entry_size) + 5] = spur.get(i).getNoOfSpursToTrack();
            List <SpurDetails> spurDetails =
                                spur.get(i).getSpurDetailsList();
            for(j = 0; j < no_Of_Spurs_For_Entry; j++) {
                rotation_value = spurDetails.get(j).getRotationValue();
                buff[(j * 4) + 6 + (i * each_Spur_entry_size)]  =
                                        (short)(rotation_value & 0xff);
                buff[(j * 4) + 7 + (i * each_Spur_entry_size)]  =
                                 (short)((rotation_value >> 8) & 0xff);
                buff[(j * 4) + 8 + (i * each_Spur_entry_size)]  =
                                (short)((rotation_value >> 16) & 0x0f);
                buff[(j * 4) + 8 + (i * each_Spur_entry_size)] |=
                 (short)(spurDetails.get(j).getLsbOfIntegrationLength() << 4);
                buff[(j * 4) + 8 + (i * each_Spur_entry_size)] |=
                      (short)(spurDetails.get(j).getFilterCoefficeint() << 5);
                buff[(j * 4) + 8 + (i * each_Spur_entry_size)] |=
                    (short)(spurDetails.get(j).getIsEnableSpur() << 7);
                buff[(j * 4) + 9 + (i * each_Spur_entry_size)]  =
                              (short)spurDetails.get(j).getSpurLevel();
            }
        }
        re = FmReceiverJNI.setSpurDataNative(fd, buff,
                        (2 + (no_of_spur_freq * each_Spur_entry_size)));
        if (re < 0)
            return false;
        return true;
    }

    public int getRadioBand(){
        return mRadioBand;
    }

    public void setRadioBand (int band){
        mRadioBand = band;
    }

    public int getEmphasis(){
        return mEmphasis;
    }

    public void setEmphasis (int emp){
        mEmphasis = emp;
    }

    public int getChSpacing (){
        return mChSpacing;
    }

    public void setChSpacing(int spacing) {
        mChSpacing = spacing;
    }

    public int getRdsStd () {
        return mRdsStd;
    }

    public void setRdsStd (int rdsStandard) {
        mRdsStd = rdsStandard;
    }

    public int getLowerLimit(){
        return mBandLowerLimit;
    }

    public void setLowerLimit(int lowLimit){
        mBandLowerLimit = lowLimit;
    }

    public int getUpperLimit(){
        return mBandUpperLimit;
    }

    public void setUpperLimit(int upLimit){
        mBandUpperLimit = upLimit;
    }

    /*
     * fmConfigure()
     * This method call v4l2 private controls to set regional settings for the
     * FM core
     */
    protected static boolean fmConfigure (final int fd, final FmConfig configSettings) {

        int re;

        Log.v (TAG, "In fmConfigure");
	re = FmReceiverJNI.setControlNative (fd, V4L2_CID_PRIVATE_TAVARUA_EMPHASIS, configSettings.getEmphasis());
        re = FmReceiverJNI.setControlNative (fd, V4L2_CID_PRIVATE_TAVARUA_RDS_STD, configSettings.getRdsStd() );
        re = FmReceiverJNI.setControlNative (fd, V4L2_CID_PRIVATE_TAVARUA_SPACING, configSettings.getChSpacing() );
        re = FmReceiverJNI.setBandNative (fd, configSettings.getLowerLimit(), configSettings.getUpperLimit());
        if (re < 0)
          return false;

        re = FmReceiverJNI.setControlNative (fd, V4L2_CID_PRIVATE_TAVARUA_REGION, configSettings.mRadioBand);
        /* setControlNative for V4L2_CID_PRIVATE_TAVARUA_REGION triggers the config change*/
        if (re < 0)
          return false;

        return true;
    }

}

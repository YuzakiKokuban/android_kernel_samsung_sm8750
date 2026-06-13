/*
 * Copyright (c) 2009,2012, The Linux Foundation. All rights reserved.
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
/**
 *
 * @hide
 */
public class FmRxRdsData {


    private String mRadioText = "";
    private String mPrgmServices;
    private String mERadioText = "";
    // false means left-right
    // true means right-left
    private boolean formatting_dir = false;
    private byte []mTagCode = new byte[2];
    private String []mTag = new String[2];
    private int tag_nums = 0;
    private static final int MAX_NUM_TAG = 2;
    private boolean rt_ert_flag;
    private int mPrgmId;
    private int mPrgmType;
    private int mFd;
    private int mECountryCode;

    /* V4L2 controls */
    private static final int V4L2_CID_PRIVATE_BASE = 0x8000000;
    private static final int V4L2_CID_PRIVATE_TAVARUA_RDSGROUP_MASK = V4L2_CID_PRIVATE_BASE + 6;
    private static final int V4L2_CID_PRIVATE_TAVARUA_RDSON = V4L2_CID_PRIVATE_BASE + 15;
    private static final int V4L2_CID_PRIVATE_TAVARUA_RDSGROUP_PROC = V4L2_CID_PRIVATE_BASE + 16;
    private static final int V4L2_CID_PRIVATE_TAVARUA_RDSD_BUF = V4L2_CID_PRIVATE_BASE + 19;
    private static final int V4L2_CID_PRIVATE_TAVARUA_PSALL = V4L2_CID_PRIVATE_BASE + 20;
    private static final int V4L2_CID_PRIVATE_TAVARUA_AF_JUMP = V4L2_CID_PRIVATE_BASE + 27;


    private static final int RDS_GROUP_RT = 0x1;
    private static final int RDS_GROUP_PS = 1 << 1;
    private static final int RDS_GROUP_AF = 1 << 2;
    private static final int RDS_AF_AUTO  = 1 << 6;
    private static final int RDS_PS_ALL   = 1 << 4;
    private static final int RDS_AF_JUMP  = 0x1;
    private static final int MAX_TAG_CODES = 64;

    private static final String LOGTAG="FmRxRdsData";


    public FmRxRdsData (int fd)
    {
      mFd = fd;
    }

    /* turn on/off RDS processing */
    public int rdsOn (boolean on)
    {

      int ret;

      Log.d(LOGTAG, "In rdsOn: RDS is " + on);

      if (on) {
        ret = FmReceiverJNI.setControlNative (mFd, V4L2_CID_PRIVATE_TAVARUA_RDSON, 1);
      }
      else {
        ret = FmReceiverJNI.setControlNative (mFd, V4L2_CID_PRIVATE_TAVARUA_RDSON, 0);
      }


      return ret;


    }

    /* process raw RDS group filtering */
    public int rdsGrpOptions (int grpMask, int buffSize, boolean rdsFilter)
    {

        int rdsFilt;
        int re;

        byte rds_group_mask = (byte)FmReceiverJNI.getControlNative(mFd, V4L2_CID_PRIVATE_TAVARUA_RDSGROUP_PROC);

        rds_group_mask &= 0xFE;


        if (rdsFilter)
          rdsFilt = 1;
        else
          rdsFilt = 0;

        rds_group_mask |= rdsFilt;

        re = FmReceiverJNI.setControlNative(mFd, V4L2_CID_PRIVATE_TAVARUA_RDSGROUP_PROC, rds_group_mask);

        if (re != 0)
          return re;

        re = FmReceiverJNI.setControlNative(mFd, V4L2_CID_PRIVATE_TAVARUA_RDSD_BUF, buffSize);

        if (re != 0)
          return re;


        re = FmReceiverJNI.setControlNative(mFd, V4L2_CID_PRIVATE_TAVARUA_RDSGROUP_MASK, grpMask);

        return re;


    }

    /* configure RT/PS/AF RDS processing */
    public int rdsOptions (int rdsMask)
    {

        int re=0;

        int rds_group_mask = FmReceiverJNI.getControlNative(mFd, V4L2_CID_PRIVATE_TAVARUA_RDSGROUP_PROC);
        byte rdsFilt = 0;
        int  psAllVal=rdsMask & RDS_PS_ALL;

        Log.d(LOGTAG, "In rdsOptions: rdsMask: " + rdsMask);

         rds_group_mask  = ((rdsMask & 0x000000FF));

        Log.d(LOGTAG, "In rdsOptions: rds_group_mask : " + rds_group_mask);
        re = FmReceiverJNI.setControlNative(mFd, V4L2_CID_PRIVATE_TAVARUA_RDSGROUP_PROC, rds_group_mask);
        return re;
    }

    /* Enable auto seek to alternate frequency */
    public int enableAFjump(boolean AFenable)
    {
      int re;
      int rds_group_mask = 0;

      Log.d(LOGTAG, "In enableAFjump: AFenable : " + AFenable);

      rds_group_mask = FmReceiverJNI.getControlNative(mFd, V4L2_CID_PRIVATE_TAVARUA_RDSGROUP_PROC);
      Log.d(LOGTAG, "Currently set rds_group_mask : " + rds_group_mask);

      if (AFenable)
        re = FmReceiverJNI.setControlNative(mFd ,V4L2_CID_PRIVATE_TAVARUA_AF_JUMP, 1);
      else
        re = FmReceiverJNI.setControlNative(mFd, V4L2_CID_PRIVATE_TAVARUA_AF_JUMP, 0);

      rds_group_mask = FmReceiverJNI.getControlNative(mFd, V4L2_CID_PRIVATE_TAVARUA_RDSGROUP_PROC);

      if (AFenable)
        Log.d(LOGTAG, "After enabling the rds_group_mask is : " + rds_group_mask);
      else
        Log.d(LOGTAG, "After disabling the rds_group_mask is : " + rds_group_mask);

      return re;
    }


    public String getRadioText () {
        return mRadioText;
    }

    public void setRadioText (String x) {

        mRadioText = x;
    }

    public String getPrgmServices () {
        return mPrgmServices;
    }
    public void setPrgmServices (String x) {
        mPrgmServices = x;
    }

    public int getPrgmId () {
        return mPrgmId;
    }
    public void setPrgmId (int x) {
        mPrgmId = x;
    }

    public int getPrgmType () {
        return mPrgmType;
    }
    public void setPrgmType (int x) {
         mPrgmType = x;
    }
    public String getERadioText () {
         return mERadioText;
    }
    public void setERadioText (String x) {
         mERadioText = x;
    }
    public void setECountryCode(int x) {
         mECountryCode = x;
    }
    public int getECountryCode() {
         return mECountryCode;
    }
    public boolean getFormatDir() {
         return formatting_dir;
    }
    public void setFormatDir(boolean dir) {
         formatting_dir = dir;
    }
    public void setTagValue (String x, int tag_num) {
        if ((tag_num > 0) && (tag_num <= MAX_NUM_TAG)) {
            mTag[tag_num - 1] = x;
            tag_nums++;
        }
    }
    public void setTagCode (byte tag_code, int tag_num) {
        if ((tag_num > 0) && (tag_num <= MAX_NUM_TAG))
            mTagCode[tag_num - 1] = tag_code;
    }
    public String getTagValue (int tag_num) {
        if ((tag_num > 0) && (tag_num <= MAX_NUM_TAG))
            return mTag[tag_num - 1];
        else
            return "";
    }
    public byte getTagCode (int tag_num) {
        if ((tag_num > 0) && (tag_num <= MAX_NUM_TAG))
            return mTagCode[tag_num - 1];
        else
            return 0;
    }
    public int getTagNums() {
        return tag_nums;
    }
    public void setTagNums(int x) {
        if ((x >= 0) && (x <= MAX_NUM_TAG))
            tag_nums = x;
    }
}

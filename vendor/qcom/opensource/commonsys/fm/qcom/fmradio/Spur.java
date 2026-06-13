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
import java.util.ArrayList;
import java.util.List;

public class Spur {

    private int SpurFreq;
    private byte NoOfSpursToTrack;
    private List <SpurDetails> spurDetailsList;

    Spur() {

    }

    Spur(int SpurFreq, byte NoOfSpursToTrack,
         List <SpurDetails> spurDetailsList) {

        this.SpurFreq = SpurFreq;
        this.NoOfSpursToTrack = NoOfSpursToTrack;
        this.spurDetailsList = spurDetailsList;
    }

    public int getSpurFreq() {
        return SpurFreq;
    }

    public void setSpurFreq(int spurFreq) {
        SpurFreq = spurFreq;
    }

    public byte getNoOfSpursToTrack() {
        return NoOfSpursToTrack;
    }

    public void setNoOfSpursToTrack(byte noOfSpursToTrack) {
        NoOfSpursToTrack = noOfSpursToTrack;
    }

    public List<SpurDetails> getSpurDetailsList() {
        return spurDetailsList;
    }

    public void setSpurDetailsList(List<SpurDetails> spurDetailsList) {
        this.spurDetailsList = spurDetailsList;
    }

    public void addSpurDetails(SpurDetails spurDetails) {
        if (spurDetailsList == null) {
            spurDetailsList = new ArrayList<SpurDetails>();
        }
        spurDetailsList.add(spurDetails);
    }

}


/*
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
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
package com.caf.fmradio;

import android.util.Log;

import java.io.*;

public class CommaSeparatedFreqFileReader implements GetNextFreqInterface {

   private BufferedReader reader;
   private String fileName;
   private String [] freqList;
   private int index;
   private static final String LOGTAG = "COMMA_SEPARATED_FREQ_PARSER";
   private boolean errorHasOcurred;

   @Override
   public int getNextFreq() {
      int freq = Integer.MAX_VALUE;

      Log.d(LOGTAG, "Inside function get freq");
      if(!errorHasOcurred) {
         if(index < freqList.length) {
            try {
                freq = (int)(Float.parseFloat(freqList[index]) * 1000);
            }catch(NumberFormatException e) {
                Log.d(LOGTAG, "Format exception");
            }
            index++;
            if(index >= freqList.length) {
               index = 0;
               readLineAndParse();
            }
            return freq;
         }else {
            return Integer.MAX_VALUE;
         }
      }else {
         return Integer.MAX_VALUE;
      }
   }

   public CommaSeparatedFreqFileReader(String fileName) {
      this.fileName = fileName;
      try {
           reader =  new BufferedReader(new FileReader(this.fileName));
           readLineAndParse();
      }catch(Exception e) {
           errorHasOcurred = true;
           Log.d(LOGTAG, "File not found");
      }
   }

   private void readLineAndParse() {
      String curLine;
      if(reader != null) {
         try {
              if((curLine = reader.readLine()) != null) {
                 freqList = curLine.split(",");
              }else {
                 reader.close();
                 reader = null;
                 errorHasOcurred = true;
              }
         }catch(Exception e) {
              errorHasOcurred = true;
         }
      }else {
         errorHasOcurred = true;
      }
   }

   @Override
   public void Stop() {
      if(reader != null) {
         try {
             reader.close();
             reader = null;
         }catch(Exception e) {
         }
         errorHasOcurred = true;
      }
   }

   @Override
   public boolean errorOccured() {
      return errorHasOcurred;
   }
}


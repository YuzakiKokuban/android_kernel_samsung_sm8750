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

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.regex.Matcher;
import android.util.Log;
import qcom.fmradio.SpurFileFormatConst.LineType;

public class SpurFileParser implements SpurFileParserInterface {

    private static final String TAG = "SPUR";

    private boolean parse(BufferedReader reader, SpurTable t) {
        int entryFound = 0;

        if (t == null ) {
            return false;
        }

        if ((reader != null)) {
            String line;
            LineType lastLine = LineType.EMPTY_LINE;
            int indexEqual;
            int SpurFreq = 0;
            byte NoOfSpursToTrack = 0;
            int RotationValue = 0;
            byte LsbOfIntegrationLength = 0;
            byte FilterCoefficeint = 0;
            byte IsEnableSpur = 0;
            byte spurLevel = 0;
            byte noOfSpursFreq = 0;
            byte mode;
            byte freqCnt = 0;

            try {
                 while(reader.ready() && (line = reader.readLine()) != null) {
                     line = removeSpaces(line);
                     System.out.println("line : " + line);
                     if (lineIsComment(line)) {
                         continue;
                     }
                     if ((entryFound == 2) && (freqCnt <= noOfSpursFreq)) {
                         if ((lastLine == LineType.EMPTY_LINE) &&
                              lineIsOfType(line, SpurFileFormatConst.SPUR_FREQ)) {

                             indexEqual = line.indexOf(SpurFileFormatConst.DELIMETER);
                             SpurFreq = Integer.parseInt(line.substring(indexEqual + 1));
                             lastLine = LineType.SPUR_FR_LINE;
                             freqCnt++;
                         } else if((lastLine == LineType.SPUR_FR_LINE) &&
                                    lineIsOfType(line, SpurFileFormatConst.SPUR_NO_OF)) {

                             indexEqual = line.indexOf(SpurFileFormatConst.DELIMETER);
                             NoOfSpursToTrack = Byte.parseByte(line.substring(indexEqual + 1));
                             Spur spur = new Spur();
                             spur.setSpurFreq(SpurFreq);
                             spur.setNoOfSpursToTrack(NoOfSpursToTrack);
                             for(int i = 0; i < 3; i++) {
                                 SpurDetails spurDetails = new SpurDetails();
                                 for(int j = 0; (reader.ready()) &&
                                   (j < SpurFileFormatConst.SPUR_DETAILS_FOR_EACH_FREQ_CNT); j++) {

                                     line = reader.readLine();
                                     line = removeSpaces(line);
                                     System.out.println("inside line: " + line);
                                     if (lineIsOfType(line,
                                            (SpurFileFormatConst.SPUR_ROTATION_VALUE + i))) {

                                         indexEqual = line.indexOf(SpurFileFormatConst.DELIMETER);
                                         RotationValue = Integer.parseInt(
                                                            line.substring(indexEqual + 1));
                                         spurDetails.setRotationValue(RotationValue);
                                     } else if(lineIsOfType(line,
                                             SpurFileFormatConst.SPUR_LSB_LENGTH + i)) {

                                         indexEqual = line.indexOf(SpurFileFormatConst.DELIMETER);
                                         LsbOfIntegrationLength = Byte.parseByte(
                                                            line.substring(indexEqual + 1));
                                         spurDetails.setLsbOfIntegrationLength(
                                                                    LsbOfIntegrationLength);
                                     } else if(lineIsOfType(line,
                                             SpurFileFormatConst.SPUR_FILTER_COEFF + i)) {

                                         indexEqual = line.indexOf(SpurFileFormatConst.DELIMETER);
                                         FilterCoefficeint = Byte.parseByte(
                                                            line.substring(indexEqual + 1));
                                         spurDetails.setFilterCoefficeint(FilterCoefficeint);
                                     } else if(lineIsOfType(line,
                                             SpurFileFormatConst.SPUR_IS_ENABLE + i)) {

                                         indexEqual = line.indexOf(SpurFileFormatConst.DELIMETER);
                                         IsEnableSpur = Byte.parseByte(
                                                         line.substring(indexEqual + 1));
                                         spurDetails.setIsEnableSpur(IsEnableSpur);
                                     } else if(lineIsOfType(line,
                                             SpurFileFormatConst.SPUR_LEVEL + i)) {

                                         indexEqual = line.indexOf(SpurFileFormatConst.DELIMETER);
                                         spurLevel = Byte.parseByte(line.substring(indexEqual + 1));
                                         spurDetails.setSpurLevel(spurLevel);
                                     }
                                 }
                                 spur.addSpurDetails(spurDetails);
                             }
                             t.InsertSpur(spur);
                             lastLine = LineType.EMPTY_LINE;
                         } else {
                             return false;
                         }
                     } else if(entryFound == 1) {
                         if (lineIsOfType(line, SpurFileFormatConst.SPUR_NUM_ENTRY)) {
                             indexEqual = line.indexOf(SpurFileFormatConst.DELIMETER);
                             noOfSpursFreq = Byte.parseByte(line.substring(indexEqual + 1));
                             t.SetspurNoOfFreq(noOfSpursFreq);
                             entryFound++;
                         } else {
                               return false;
                         }
                     } else {
                         if (lineIsOfType(line, SpurFileFormatConst.SPUR_MODE)) {
                             indexEqual = line.indexOf(SpurFileFormatConst.DELIMETER);
                             mode = Byte.parseByte(line.substring(indexEqual + 1));
                             t.SetMode(mode);
                             entryFound++;
                         } else {
                             return false;
                         }
                     }
                 } // END of while
            } catch (NumberFormatException e) {
                 Log.d(TAG, "NumberFormatException");
                 e.printStackTrace();
                 return false;
            } catch (IOException e) {
                 Log.d(TAG, "IOException");
                 e.printStackTrace();
                 return false;
            }
        } else {
            return false;
        }
        return true;
    }

    private String removeSpaces(String s) {
        Matcher matcher = SpurFileFormatConst.SPACE_PATTERN.matcher(s);
        String newLine = matcher.replaceAll("");
        return newLine;
    }

    private boolean lineIsOfType(String line, String lineType) {
        int indexEqual;

        try {
             indexEqual = line.indexOf(SpurFileFormatConst.DELIMETER);
             if ((indexEqual >= 0) && indexEqual < line.length()) {
                 if (line.startsWith(lineType)) {
                     int num = Integer.parseInt(line.substring(indexEqual + 1));
                 } else {
                     return false;
                 }
             } else {
                 return false;
             }
        } catch (NumberFormatException e) {
             return false;
        }
        return true;
    }

    private boolean lineIsComment(String s) {
        if ((s == null) || (s == "") || (s == " ") || s.length() == 0) {
            return true;
        } else if(s.charAt(0) == SpurFileFormatConst.COMMENT) {
            return true;
        } else {
            return false;
        }
    }

    @Override
    public SpurTable GetSpurTable(String fileName) {
        BufferedReader reader = null;
        SpurTable t = new SpurTable();
        try {
             reader = new BufferedReader(new InputStreamReader(new FileInputStream(fileName)));
             parse(reader, t);
             reader.close();
        } catch (IOException e) {
             e.printStackTrace();
        }
        return t;
    }

}

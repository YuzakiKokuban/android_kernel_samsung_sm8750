/*
 * Copyright (c) 2009-2014, The Linux Foundation. All rights reserved.
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

import java.util.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.ListIterator;
import java.util.Locale;
import java.util.Map;

import android.content.Context;
import android.content.SharedPreferences;
import qcom.fmradio.FmReceiver;
import qcom.fmradio.FmConfig;
import android.os.SystemProperties;
import android.util.Log;

public class FmSharedPreferences
{
   public static final int REGIONAL_BAND_NORTH_AMERICA   = 0;
   public static final int REGIONAL_BAND_EUROPE          = 1;
   public static final int REGIONAL_BAND_JAPAN           = 2;
   public static final int REGIONAL_BAND_JAPAN_WIDE      = 3;
   public static final int REGIONAL_BAND_AUSTRALIA       = 4;
   public static final int REGIONAL_BAND_AUSTRIA         = 5;
   public static final int REGIONAL_BAND_BELGIUM         = 6;
   public static final int REGIONAL_BAND_BRAZIL          = 7;
   public static final int REGIONAL_BAND_CHINA           = 8;
   public static final int REGIONAL_BAND_CZECH           = 9;
   public static final int REGIONAL_BAND_DENMARK         = 10;
   public static final int REGIONAL_BAND_FINLAND         = 11;
   public static final int REGIONAL_BAND_FRANCE          = 12;
   public static final int REGIONAL_BAND_GERMANY         = 13;
   public static final int REGIONAL_BAND_GREECE          = 14;
   public static final int REGIONAL_BAND_HONGKONG        = 15;
   public static final int REGIONAL_BAND_INDIA           = 16;
   public static final int REGIONAL_BAND_IRELAND         = 17;
   public static final int REGIONAL_BAND_ITALY           = 18;
   public static final int REGIONAL_BAND_KOREA           = 19;
   public static final int REGIONAL_BAND_MEXICO          = 20;
   public static final int REGIONAL_BAND_NETHERLANDS     = 21;
   public static final int REGIONAL_BAND_NEWZEALAND      = 22;
   public static final int REGIONAL_BAND_NORWAY          = 23;
   public static final int REGIONAL_BAND_POLAND          = 24;
   public static final int REGIONAL_BAND_PORTUGAL        = 25;
   public static final int REGIONAL_BAND_RUSSIA          = 26;
   public static final int REGIONAL_BAND_SINGAPORE       = 27;
   public static final int REGIONAL_BAND_SLOVAKIA        = 28;
   public static final int REGIONAL_BAND_SPAIN           = 29;
   public static final int REGIONAL_BAND_SWITZERLAND     = 30;
   public static final int REGIONAL_BAND_SWEDEN          = 31;
   public static final int REGIONAL_BAND_TAIWAN          = 32;
   public static final int REGIONAL_BAND_TURKEY          = 33;
   public static final int REGIONAL_BAND_UNITEDKINGDOM   = 34;
   public static final int REGIONAL_BAND_UNITED_STATES   = 35;
   public static final int REGIONAL_BAND_USER_DEFINED    = 36;

   public static final int RECORD_DUR_INDEX_0_VAL        = 5;
   public static final int RECORD_DUR_INDEX_1_VAL       = 15;
   public static final int RECORD_DUR_INDEX_2_VAL       = 30;
   public static final int RECORD_DUR_INDEX_3_VAL       = -1;

   private static final String LOGTAG = "FmSharedPreferences";

   private static final String SHARED_PREFS = "fmradio_prefs";
   private static final String LIST_NUM = "list_number";
   private static final String LIST_NAME = "list_name";
   private static final String STATION_NAME = "station_name";
   private static final String STATION_FREQUENCY = "station_freq";
   private static final String STATION_ID = "station_id";
   private static final String STATION_PTY = "station_pty";
   private static final String STATION_RDS = "station_rds";
   private static final String STATION_NUM = "preset_number";

   private static final String FMCONFIG_COUNTRY = "fmconfig_country";
   //private static final String FMCONFIG_BAND = "fmconfig_band";
   private static final String FMCONFIG_MIN = "fmconfig_min";
   private static final String FMCONFIG_MAX = "fmconfig_max";
   private static final String FMCONFIG_STEP = "fmconfig_step";
   //private static final String FMCONFIG_EMPH = "fmconfig_emphasis";
   //private static final String FMCONFIG_RDSSTD = "fmconfig_rdsstd";
   /* Storage key String */
   private static final String LAST_LIST_INDEX = "last_list_index";
   public static final int MAX_NUM_TAG_TYPES = 64;
   public static final int NUM_TAG_CATEGORY = 8;
   public static final String []TAG_CATEGORIES = { "DUMMY", "ITEM", "INFO", "PROGRAMME",
                                                      "INTERACTIVITY", "RFU", "PRIVATE_CLASSES",
                                                      "DESCRIPTOR" };
   public static final int [][]TAG_CATEGORY_RANGE  = { {0, 0}, {1, 11}, {12, 30},
                                                         {31, 40}, {41, 53}, {54, 55},
                                                         {56, 58}, {59, 63} };
   public static final String []TAG_NAMES = { "DUMMY", "ITEM.TITLE", "ITEM.ALBUM", "ITEM.TRACKNUM",
                                                  "ITEM.ARTIST", "ITEM.COMPOSITION", "ITEM.MOVEMENT",
                                                  "ITEM.CONDUCTOR", "ITEM.COMPOSER", "ITEM.BAND", "ITEM.COMMENT",
                                                  "ITEM.GENERE", "INFO.NEWS", "INFO.NEWS_LOCAL", "INFO.STOCK",
                                                  "INFO.SPORT", "INFO.LOTTERY", "INFO.HOROSCOPE",
                                                  "INFO.DAILY_DIVERSION", "INFO.HEALTH", "INFO.EVENT",
                                                  "INFO.SCENE", "INFO.CINEMA", "INFO.TV", "INFO.DATE_TIME",
                                                  "INFO.WEATHER", "INFO.TRAFFIC", "INFO.ALARM", "INFO.ADS",
                                                  "INFO.URL", "INFO.OTHER", "PROGRAMME.STATIONNAME_SHORT",
                                                  "PROGRAMME.NOW", "PROGRAMME.NEXT", "PROGRAMME.PART",
                                                  "PROGRAMME.HOST", "PROGRAMME.EDITORIAL_STAFF",
                                                  "PROGRAMME.FREQUENCY", "PROGRAMME.HOMEPAGE",
                                                  "PROGRAMME.SUBCHANNEL", "PHONE.HOTLINE", "PHONE.STUDIO",
                                                  "PHONE.OTHER", "SMS.STUDIO", "SMS.OTHER", "EMAIL.HOTLINE",
                                                  "EMAIL.STUDIO", "EMAIL.OTHER", "MMS.OTHER", "CHAT",
                                                  "CHAT.CENTRE", "VOTE.QUESTION", "VOTE.CENTRE", "RFU.1",
                                                  "RFU.2", "PRIVATE.1", "PRIVATE.2", "PRIVATE.3",
                                                  "PLACE", "APPOINTMENT", "IDENTIFIER", "PURCHASE", "GET_DATA" };

   private static final String PREF_LAST_TUNED_FREQUENCY = "last_frequency";
   private static final String LAST_RECORD_DURATION = "last_record_duration";
   private static String  LAST_AF_JUMP_VALUE = "last_af_jump_value";
   private static final String AUDIO_OUTPUT_MODE = "audio_output_mode";
   private static Map<String, String> mNameMap = new HashMap<String, String>();
   private static List<PresetList> mListOfPlists = new ArrayList<PresetList>();
   public static Set[] tagList = new TreeSet[FmSharedPreferences.MAX_NUM_TAG_TYPES];
   public static int num_tags = 0;
   private static FmConfig mFMConfiguration;

   private static final String DEFAULT_NO_NAME = "";
   public static final int DEFAULT_NO_FREQUENCY = 98100;
   private static final int DEFAULT_NO_PTY = 0;
   private static final int DEFAULT_NO_STATIONID = 0;
   private static final int DEFAULT_NO_RDSSUP = 0;
   private static CharSequence[] mListEntries;
   private static CharSequence[] mListValues;
   private static int mListIndex;
   private Context mContext;
   private static int mTunedFrequency = 98100;
   private static int mBandMinFreq = 76000;
   private static int mBandMaxFreq = 108000;
   private static int mChanSpacing = 0;
   private static int mFrequencyBand_Stepsize = 200;

   private static int mCountry = 0;
   /* true = Stereo and false = "force Mono" even if Station is transmitting a
    * Stereo signal
    */
   private static boolean mAudioOutputMode = true;
   private static boolean mAFAutoSwitch = true;
   private static int mRecordDuration = 0;
   private static int mLastAudioMode = -1;
   private static boolean mSpecialCarrierFlag = false;

   FmSharedPreferences(Context context){
      mContext = context.getApplicationContext();
      mFMConfiguration = new FmConfig();
      Load();
   }

   public static void removeStation(int listIndex, int stationIndex){
      if (listIndex < getNumList())
      {
         mListOfPlists.get(listIndex).removeStation(stationIndex);
      }
   }
   public static void removeStation(int listIndex, PresetStation station){
      if (listIndex < getNumList())
      {
         mListOfPlists.get(listIndex).removeStation(station);
      }
   }

   public static void setListName(int listIndex, String name){
      if (listIndex < getNumList())
      {
         mListOfPlists.get(listIndex).setName(name);
      }
   }

   public static void setStationName(int listIndex, int stationIndex, String name){
      if (listIndex < getNumList())
      {
         mListOfPlists.get(listIndex).setStationName(stationIndex, name);
      }
   }

   public static String getListName(int listIndex){
      String name = "";
      addListIfEmpty(listIndex);
      if (listIndex < getNumList())
      {
         name= mListOfPlists.get(listIndex).getName();
      }
      return name;
   }

   public static String getStationName(int listIndex, int stationIndex){
      String name = "";
      if (listIndex < getNumList())
      {
         name = mListOfPlists.get(listIndex).getStationName(stationIndex);
      }
      return name;
   }

   public static double getStationFrequency(int listIndex, int stationIndex){
      double frequency = 0;
      if (listIndex < getNumList())
      {
         frequency =  mListOfPlists.get(listIndex).getStationFrequency(stationIndex);
      }
      return frequency;
   }

   public static PresetList getStationList(int listIndex){
      if (listIndex < getNumList())
      {
         return mListOfPlists.get(listIndex);
      }
      return null;
   }

   public static PresetStation getselectedStation(){
      int listIndex = getCurrentListIndex();
      PresetStation station = null;
      if (listIndex < getNumList())
      {
         station = mListOfPlists.get(listIndex).getSelectedStation();
      }
      return station;
   }

   public static PresetStation getStationInList(int index){
      int listIndex = getCurrentListIndex();
      PresetStation station = null;
      if (listIndex < getNumList())
      {
         station = mListOfPlists.get(listIndex).getStationFromIndex(index);
      }
      return station;
   }
   public static PresetStation getStationFromFrequency(int frequency){
      int listIndex = getCurrentListIndex();
      PresetStation station = null;
      if(listIndex < getNumList())
      {
         station = mListOfPlists.get(listIndex).getStationFromFrequency(frequency);
      }
      return station;
   }

   public static PresetStation selectNextStation(){
      int listIndex = getCurrentListIndex();
      PresetStation station = null;
      if (listIndex < getNumList())
      {
         station = mListOfPlists.get(listIndex).selectNextStation();
      }
      return station;
   }

   public static PresetStation selectPrevStation(){
      int listIndex = getCurrentListIndex();
      PresetStation station = null;
      if (listIndex < getNumList())
      {
         station = mListOfPlists.get(listIndex).selectPrevStation();
      }
      return station;
   }

   public static void selectStation(PresetStation station){
      int listIndex = getCurrentListIndex();
      if (listIndex < getNumList())
      {
         mListOfPlists.get(listIndex).selectStation(station);
      }
   }

   public static int getNumList(){
      return mListOfPlists.size();
   }

   public static int getCurrentListIndex(){
      return mListIndex;
   }

   public static void setListIndex(int index){
      mListIndex = index;
   }

   public static Map<String, String> getNameMap(){
      return mNameMap;
   }

   private static void addListIfEmpty(int listIndex){
      if ((listIndex < 1) && (getNumList() == 0))
      {
         createPresetList("FM");
      }
   }

   public static void addStation(String name, int freq, int listIndex){
      /* If no lists exists and a new station is added, add a new Preset List
       * if "listIndex" requested was "0"
       */
      addListIfEmpty(listIndex);
      if (getNumList() > listIndex)
      {
         mListOfPlists.get(listIndex).addStation(name, freq);
      }
   }

   /** Add "station" into the Preset List indexed by "listIndex" */
   public static void addStation(int listIndex, PresetStation station){
      /* If no lists exists and a new station is added, add a new Preset List
       * if "listIndex" requested was "0"
       */
      addListIfEmpty(listIndex);
      if (getNumList() > listIndex)
      {
         mListOfPlists.get(listIndex).addStation(station);
      }
   }
   public static void addTags(int index, String s) {
     if ((index >= 0) && (index <FmSharedPreferences.MAX_NUM_TAG_TYPES) &&
             s != null && !s.isEmpty()) {
          if(tagList[index] == null) {
             tagList[index] = new TreeSet<String>();
          }
          if (tagList[index].add(s))
              num_tags++;
     }
   }
   public static void clearTags() {
      for(int i = 0; i <FmSharedPreferences.MAX_NUM_TAG_TYPES; i++) {
          if(tagList[i] != null) {
             tagList[i].clear();
             Log.d(LOGTAG, "cleared tags of type" + i);
          }
      }
      num_tags = 0;
   }

   /** Does "station" already exist in the Preset List indexed by "listIndex" */
   public static boolean sameStationExists(int listIndex, PresetStation station){
      boolean exists = false;
      if (getNumList() > listIndex)
      {
         exists = mListOfPlists.get(listIndex).sameStationExists(station);
      }
      return exists;
   }

   /** Does "station" already exist in the current Preset List*/
   public static boolean sameStationExists( PresetStation station){
      int listIndex = getCurrentListIndex();
      boolean exists = false;
      if (getNumList() > listIndex)
      {
         exists = mListOfPlists.get(listIndex).sameStationExists(station);
      }
      return exists;
   }

   /** Does "station" already exist in the current Preset List*/
   public static int getListStationCount( ){
      int listIndex = getCurrentListIndex();
      int numStations = 0;
      if (getNumList() > listIndex)
      {
         numStations = mListOfPlists.get(listIndex).getStationCount();
      }
      return numStations;
   }

   public static void renamePresetList(String newName, int listIndex){
      PresetList curList =    mListOfPlists.get(listIndex);
      if (curList != null)
      {
         String oldListName = curList.getName();
         curList.setName(newName);
         String index = mNameMap.get(oldListName);
         mNameMap.remove(oldListName);
         mNameMap.put((String) newName, index);
         repopulateEntryValueLists();
      }
   }

   /* Returns the index of the list just created */
   public static int createPresetList(String name) {
      int numLists = mListOfPlists.size();
      mListOfPlists.add(new PresetList(name));
      String index = String.valueOf(numLists);
      mNameMap.put(name, index);
      repopulateEntryValueLists();
      return numLists;
   }


   public static void createFirstPresetList(String name) {
      mListIndex = 0;
      createPresetList(name);
   }

   public static CharSequence[] repopulateEntryValueLists() {
      ListIterator<PresetList> presetIter;
      presetIter = mListOfPlists.listIterator();
      int numLists = mListOfPlists.size();

      mListEntries = new CharSequence[numLists];
      mListValues = new CharSequence[numLists];
      for (int i = 0; i < numLists; i++)
      {
         PresetList temp = presetIter.next();
         mListEntries[i] = temp.getName();
         mListValues[i] = temp.getName();
      }
      return mListEntries;
   }

   public static List<PresetList> getPresetLists() {
      return mListOfPlists;
   }


   public void  Load(){
      Log.d(LOGTAG, "Load preferences ");
      if(mContext == null)
      {
         return;
      }
      mSpecialCarrierFlag = mContext.getResources().getBoolean(
              R.bool.def_fm_special_carrier_enabled);
      SharedPreferences sp = mContext.getSharedPreferences(SHARED_PREFS, Context.MODE_PRIVATE);
      mTunedFrequency = sp.getInt(PREF_LAST_TUNED_FREQUENCY, DEFAULT_NO_FREQUENCY);
      mRecordDuration = sp.getInt(LAST_RECORD_DURATION, RECORD_DUR_INDEX_0_VAL);
      mAFAutoSwitch = sp.getBoolean(LAST_AF_JUMP_VALUE, true);
      mAudioOutputMode = sp.getBoolean(AUDIO_OUTPUT_MODE, true);

      if(sp.getInt(FMCONFIG_COUNTRY, 0) == REGIONAL_BAND_USER_DEFINED) {
         mBandMinFreq = sp.getInt(FMCONFIG_MIN, mBandMinFreq);
         mBandMaxFreq = sp.getInt(FMCONFIG_MAX, mBandMaxFreq);
         mChanSpacing = sp.getInt(FMCONFIG_STEP, mChanSpacing);
      }

      int num_lists = sp.getInt(LIST_NUM, 1);
      if (mListOfPlists.size() == 0) {

         for (int listIter = 0; listIter < num_lists; listIter++) {
             String listName = sp.getString(LIST_NAME + listIter, "FM - " + (listIter+1));
             int numStations = sp.getInt(STATION_NUM + listIter, 1);
             if (listIter == 0) {
                 createFirstPresetList(listName);
             } else {
                 createPresetList(listName);
             }

             PresetList curList = mListOfPlists.get(listIter);
             for (int stationIter = 0; stationIter < numStations; stationIter++) {
                  String stationName = sp.getString(STATION_NAME + listIter + "x" + stationIter,
                                                      DEFAULT_NO_NAME);
                  int stationFreq = sp.getInt(STATION_FREQUENCY + listIter + "x" + stationIter,
                                                   DEFAULT_NO_FREQUENCY);
                  PresetStation station = curList.addStation(stationName, stationFreq);

                  int stationId = sp.getInt(STATION_ID + listIter + "x" + stationIter,
                                              DEFAULT_NO_STATIONID);
                  station.setPI(stationId);

                  int pty = sp.getInt(STATION_PTY + listIter + "x" + stationIter, DEFAULT_NO_PTY);
                  station.setPty(pty);

                  int rdsSupported = sp.getInt(STATION_RDS + listIter + "x" + stationIter,
                                                 DEFAULT_NO_RDSSUP);
                  if (rdsSupported != 0) {
                      station.setRDSSupported(true);
                  } else {
                      station.setRDSSupported(false);
                  }
             }
         }
      }
      /* Load Configuration */
      if (Locale.getDefault().equals(Locale.CHINA)) {
          setCountry(sp.getInt(FMCONFIG_COUNTRY, REGIONAL_BAND_CHINA));
        } else if (mContext.getResources()
                .getBoolean(R.bool.def_fm_country_location_enabled)) {
            setCountry(sp.getInt(FMCONFIG_COUNTRY, REGIONAL_BAND_INDIA));
      } else {
          setCountry(sp.getInt(FMCONFIG_COUNTRY, REGIONAL_BAND_NORTH_AMERICA));
      }
      /* Last list the user was navigating */
      mListIndex = sp.getInt(LAST_LIST_INDEX, 0);
      if(mListIndex >= num_lists)
      {
         mListIndex=0;
      }
   }

   public void Save() {
      if(mContext == null)
      {
         return;
      }
      Log.d(LOGTAG, "Save preferences ");

      int numLists = mListOfPlists.size();
      SharedPreferences sp = mContext.getSharedPreferences(SHARED_PREFS, Context.MODE_PRIVATE);
      SharedPreferences.Editor ed = sp.edit();

      ed.putInt(PREF_LAST_TUNED_FREQUENCY, mTunedFrequency);

      ed.putInt(LIST_NUM, numLists);
      /* Last list the user was navigating */
      ed.putInt(LAST_LIST_INDEX, mListIndex);

      for (int listIter = 0; listIter < numLists; listIter++)
      {
         PresetList curList = mListOfPlists.get(listIter);
         ed.putString(LIST_NAME + listIter, curList.getName());
         int numStations = curList.getStationCount();
         ed.putInt(STATION_NUM + listIter, numStations);
         int numStation = 0;
         for (int stationIter = 0; stationIter < numStations; stationIter++)
         {
            PresetStation station = curList.getStationFromIndex(stationIter);
            if (station != null)
            {
               ed.putString(STATION_NAME + listIter + "x" + numStation,
                            station.getName());
               ed.putInt(STATION_FREQUENCY + listIter + "x" + numStation,
                         station.getFrequency());
               ed.putInt(STATION_ID + listIter + "x" + numStation,
                         station.getPI());
               ed.putInt(STATION_PTY + listIter + "x" + numStation,
                         station.getPty());
               ed.putInt(STATION_RDS + listIter + "x" + numStation,
                         (station.getRDSSupported() == true? 1:0));
               numStation ++;
            }
         }
      }

      /* Save Configuration */
      ed.putInt(FMCONFIG_COUNTRY, mCountry);
      if(mCountry == REGIONAL_BAND_USER_DEFINED) {
         ed.putInt(FMCONFIG_MIN, mBandMinFreq);
         ed.putInt(FMCONFIG_MAX, mBandMaxFreq);
         ed.putInt(FMCONFIG_STEP, mChanSpacing);
      }
      ed.putInt(LAST_RECORD_DURATION, mRecordDuration);
      ed.putBoolean(LAST_AF_JUMP_VALUE, mAFAutoSwitch);
      ed.putBoolean(AUDIO_OUTPUT_MODE, mAudioOutputMode);
      ed.commit();
   }

   public static void SetDefaults() {
      mListIndex = 0;
      mListOfPlists.clear();
      if (Locale.getDefault().equals(Locale.CHINA)){
          setCountry(REGIONAL_BAND_CHINA);
          //Others set north America.
      } else {
          setCountry(REGIONAL_BAND_NORTH_AMERICA);
      }
   }

   public static void removeStationList(int listIndex) {
      mListIndex = listIndex;
      PresetList toRemove = mListOfPlists.get(mListIndex);

      mNameMap.remove(toRemove.getName());
      mListOfPlists.remove(mListIndex);
      int numLists = mListOfPlists.size();

      /* Remove for others */
      for (int i = mListIndex; i < numLists; i++)
      {
         PresetList curList = mListOfPlists.get(i);
         if (curList!=null)
         {
            String listName = curList.getName();
            /* Removals */
            mNameMap.remove(listName);
            mNameMap.put(listName, String.valueOf(i));
         }
      }
      mListIndex = 0;
      repopulateEntryValueLists();
   }

   public static void setTunedFrequency(int frequency) {
      mTunedFrequency = frequency;
   }

   public static int getTunedFrequency() {
      return mTunedFrequency;
   }

   public static int getNextTuneFrequency(int frequency) {
      int nextFrequency = (frequency + mFrequencyBand_Stepsize);
      if (nextFrequency > getUpperLimit())
      {
          nextFrequency = getLowerLimit();
      }
      return nextFrequency;
   }

   public static int getNextTuneFrequency() {
      int nextFrequency = (mTunedFrequency + mFrequencyBand_Stepsize);
      if (nextFrequency > getUpperLimit())
      {
          nextFrequency = getLowerLimit();
      }
      return nextFrequency;
   }

   public static int getPrevTuneFrequency(int frequency) {
      int prevFrequency = (frequency - mFrequencyBand_Stepsize);
      if (prevFrequency < getLowerLimit())
      {
          prevFrequency = getUpperLimit();
      }
      return prevFrequency;
   }

   public static int getPrevTuneFrequency() {
      int prevFrequency = (mTunedFrequency - mFrequencyBand_Stepsize);
      if (prevFrequency < getLowerLimit())
      {
         prevFrequency = getUpperLimit();
      }
      return prevFrequency;
   }

   /**
    * @param mFMConfiguration the mFMConfiguration to set
    */
   public static void setFMConfiguration(FmConfig mFMConfig) {
      FmSharedPreferences.mFMConfiguration = mFMConfig;
   }

   /**
    * @return the mFMConfiguration
    */
   public static FmConfig getFMConfiguration() {
      return mFMConfiguration;
   }

   public static void setRadioBand(int band)
   {
      switch (band)
      {
      case FmReceiver.FM_JAPAN_WIDE_BAND:
         {
            mFrequencyBand_Stepsize = 50;
            mFMConfiguration.setLowerLimit(76000);
            mFMConfiguration.setUpperLimit(108000);
            break;
         }
      case FmReceiver.FM_JAPAN_STANDARD_BAND:
         {
            mFrequencyBand_Stepsize = 100;
            mFMConfiguration.setLowerLimit(76000);
            mFMConfiguration.setUpperLimit(90000);
            break;
         }
      case FmReceiver.FM_USER_DEFINED_BAND:
         {
            break;
         }
      case FmReceiver.FM_US_BAND:
      case FmReceiver.FM_EU_BAND:
      default:
         {
            band =  FmReceiver.FM_US_BAND;
            mFMConfiguration.setLowerLimit(87500);
            mFMConfiguration.setUpperLimit(107900);
            mFrequencyBand_Stepsize = 100;
         }
      }
      mFMConfiguration.setRadioBand(band);
   }

   public static int getRadioBand()
   {
      return mFMConfiguration.getRadioBand();
   }

   public static void setChSpacing(int spacing)
   {
      if( (spacing >= FmReceiver.FM_CHSPACE_200_KHZ)
          && (spacing <= FmReceiver.FM_CHSPACE_50_KHZ))
      {
         mFrequencyBand_Stepsize = 200;
         switch (spacing)
         {
         case FmReceiver.FM_CHSPACE_100_KHZ:
            {
               mFrequencyBand_Stepsize = 100;
               break;
            }
         case FmReceiver.FM_CHSPACE_50_KHZ:
            {
               mFrequencyBand_Stepsize = 50;
               break;
            }
         }
         mChanSpacing = spacing;
         mFMConfiguration.setChSpacing(spacing);
      }
   }

   public static int getChSpacing()
   {
      return mFMConfiguration.getChSpacing();
   }

   public static void setRdsStd(int std)
   {
      if((std>=FmReceiver.FM_RDS_STD_RBDS)
          && (std<=FmReceiver.FM_RDS_STD_NONE))
      {
         mFMConfiguration.setRdsStd(std);
      }
   }

   public static int getRdsStd()
   {
      return mFMConfiguration.getRdsStd();
   }

   /* North America */
   public static boolean isRDSStd()
   {
      return(FmReceiver.FM_RDS_STD_RDS == mFMConfiguration.getRdsStd());
   }

   public static boolean isRBDSStd()
   {
      return(FmReceiver.FM_RDS_STD_RBDS == mFMConfiguration.getRdsStd());
   }

   public static void setEmphasis(int emph)
   {
      if((emph>=FmReceiver.FM_DE_EMP75)
          && (emph<=FmReceiver.FM_DE_EMP50))
      {
         mFMConfiguration.setEmphasis(emph);
      }
   }

   public static int getEmphasis()
   {
      return mFMConfiguration.getEmphasis();
   }

   public static int getUpperLimit()
   {
      return mFMConfiguration.getUpperLimit();
   }

   public static int getLowerLimit()
   {
      return mFMConfiguration.getLowerLimit();
   }
   public static int getFrequencyStepSize() {
      return mFrequencyBand_Stepsize;
   }
   public static void setLowerLimit(int lowLimit){
      mFMConfiguration.setLowerLimit(lowLimit);
      if(mCountry == REGIONAL_BAND_USER_DEFINED) {
         mBandMinFreq = lowLimit;
      }
   }

   public static void setUpperLimit(int upLimit){
      mFMConfiguration.setUpperLimit(upLimit);
      if(mCountry == REGIONAL_BAND_USER_DEFINED) {
         mBandMaxFreq = upLimit;
      }
   }

   public static void setCountry(int nCountryCode){

      // Default: 87500  TO 10800 IN 100 KHZ STEPS
      mFMConfiguration.setRadioBand(FmReceiver.FM_USER_DEFINED_BAND);
      mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_100_KHZ);
      mFMConfiguration.setEmphasis(FmReceiver.FM_DE_EMP50);
      mFMConfiguration.setRdsStd(FmReceiver.FM_RDS_STD_RDS);
      mFMConfiguration.setLowerLimit(87500);
      mFMConfiguration.setUpperLimit(108000);

      switch(nCountryCode)
      {
        case REGIONAL_BAND_NORTH_AMERICA:
        {
          /*NORTH_AMERICA : 87500 TO 108000 IN 200 KHZ STEPS*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_US_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_200_KHZ);
          mFMConfiguration.setEmphasis(FmReceiver.FM_DE_EMP75);
          mFMConfiguration.setRdsStd(FmReceiver.FM_RDS_STD_RBDS);
          mFMConfiguration.setLowerLimit(87500);
          mFMConfiguration.setUpperLimit(107900);
          mFrequencyBand_Stepsize = 200;
          break;
        }
        case REGIONAL_BAND_EUROPE:
        {
          /*EUROPE/Default : 87500 TO 10800 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }

        case REGIONAL_BAND_JAPAN:
        {
          /*JAPAN : 76000  TO 90000 IN 100 KHZ STEPS*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_JAPAN_STANDARD_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_100_KHZ);
          mFMConfiguration.setEmphasis(FmReceiver.FM_DE_EMP75);
          mFMConfiguration.setLowerLimit(76000);
          mFMConfiguration.setUpperLimit(90000);
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_JAPAN_WIDE:
        {
          /*JAPAN_WB : 90000 TO 108000 IN 50 KHZ STEPS*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_JAPAN_WIDE_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_50_KHZ);
          mFMConfiguration.setEmphasis(FmReceiver.FM_DE_EMP75);
          mFMConfiguration.setLowerLimit(90000);
          mFMConfiguration.setUpperLimit(108000);
          mFrequencyBand_Stepsize = 50;
          break;
        }

        /* Country specific */
        case REGIONAL_BAND_AUSTRALIA:
        {
          /*AUSTRALIA : 87700 TO 108000 IN 100 KHZ STEPS*/
          mFMConfiguration.setLowerLimit(87700);
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_AUSTRIA:
        {
          /*AUSTRIA : 87500 TO 108000 IN 50 KHZ STEPS*/
          mFMConfiguration.setEmphasis(FmReceiver.FM_DE_EMP50);
          mFrequencyBand_Stepsize = 50;
          break;
        }
        case REGIONAL_BAND_BELGIUM:
       {
         /*BELGIUM : 87500 TO 108000 IN 100 KHZ STEPS*/
         mFrequencyBand_Stepsize = 100;
         break;
       }
       case REGIONAL_BAND_BRAZIL:
       {
         /*BRAZIL : 87500 TO 108000 IN 200 KHZ STEP*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_US_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_200_KHZ);
          mFMConfiguration.setLowerLimit(87500);
          mFMConfiguration.setUpperLimit(107900);
          mFrequencyBand_Stepsize = 200;
          break;
        }
        case REGIONAL_BAND_CHINA:
        {
          /*CHINA : 87000 TO 108000 IN 100 KHZ STEPS*/
          mFMConfiguration.setLowerLimit(87000);
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_CZECH:
        {
          /*CZECH : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_DENMARK:
        {
          /*DENMARK : 87500 TO 108000 IN 50 KHZ STEPS*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_EU_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_50_KHZ);
          mFrequencyBand_Stepsize = 50;
          break;
        }
        case REGIONAL_BAND_FINLAND:
        {
          /*FINLAND : 87500  TO 108000  IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_FRANCE:
        {
          /* FRANCE : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_GERMANY:
          /*GERMANY : 87500 TO 108000 IN 50 KHZ STEPS*/
        case REGIONAL_BAND_GREECE:
          /*GREECE : 87500 TO 108000 IN 50 KHZ STEPS*/
        {
          mFMConfiguration.setRadioBand(FmReceiver.FM_EU_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_50_KHZ);
          mFrequencyBand_Stepsize = 50;
          break;
        }
        case REGIONAL_BAND_HONGKONG:
        {
          /*HONG KONG : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_INDIA:
        {
          if (mSpecialCarrierFlag) {
              /*87500 TO 108000 IN 100 KHZ STEPS*/
              mFMConfiguration.setLowerLimit(87500);
              mFMConfiguration.setUpperLimit(108000);
          } else {
              /*91000 TO 106400 IN 100 KHZ STEPS*/
              mFMConfiguration.setLowerLimit(91000);
              mFMConfiguration.setUpperLimit(106400);
          }
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_IRELAND:
        {
          /*IRELAND : 87500 TO 108000 IN 50 KHZ STEPS*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_EU_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_50_KHZ);
          mFrequencyBand_Stepsize = 50;
          break;
        }
        case REGIONAL_BAND_ITALY:
        {
          /*ITALY : 87500 TO 108000 IN 50 KHZ STEPS*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_EU_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_50_KHZ);
          mFrequencyBand_Stepsize = 50;
          break;
        }
        case REGIONAL_BAND_KOREA:
        {
          /*KOREA : 87500 TO 108000 IN 200 KHZ STEPS*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_US_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_200_KHZ);
          mFMConfiguration.setUpperLimit(107900);
          mFrequencyBand_Stepsize = 200;
          break;
        }
        case REGIONAL_BAND_MEXICO:
        {
          /*MEXICO : 88100 TO 107900 IN 200 KHZ STEPS*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_US_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_200_KHZ);
          mFMConfiguration.setEmphasis(FmReceiver.FM_DE_EMP75);
          mFMConfiguration.setRdsStd(FmReceiver.FM_RDS_STD_RBDS);
          mFMConfiguration.setLowerLimit(88100);
          mFMConfiguration.setUpperLimit(107900);
          mFrequencyBand_Stepsize = 200;
          break;
        }
        case REGIONAL_BAND_NETHERLANDS:
        {
          /*NETHERLANDS : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_NEWZEALAND:
        {
          /*NEW ZEALAND : 88000 TO 107000 IN 100 KHZ STEPS*/
          mFMConfiguration.setLowerLimit(88000);
          mFMConfiguration.setUpperLimit(107000);
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_NORWAY:
        {
          /*NORWAY : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_POLAND:
        {
          /*POLAND : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_EU_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_100_KHZ);
          mFMConfiguration.setLowerLimit(87500);
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_PORTUGAL:
        {
          /*PORTUGAL : 87500 TO 108000 IN 50 KHZ STEPS*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_EU_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_50_KHZ);
          mFrequencyBand_Stepsize = 50;
          break;
        }
        case REGIONAL_BAND_RUSSIA:
        {
          /*RUSSIA : 87500 TO 108000  IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_SINGAPORE:
        {
          /*SINGAPORE : 88000 TO 108000 IN 100 KHZ STEPS*/
          mFMConfiguration.setLowerLimit(88000);
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_SLOVAKIA:
        {
          /*SLOVAKIA : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_SPAIN:
        {
          /*SPAIN : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_SWITZERLAND:
        {
          /*SWITZERLAND : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_SWEDEN:
        {
          /*SWEDEN : 87500 TO 108000  IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_TAIWAN:
        {
          /*TAIWAN : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_TURKEY:
        {
          /*TURKEY : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_UNITEDKINGDOM:
        {
          /*UNITED KINGDOM : 87500 TO 108000 IN 100 KHZ STEPS*/
          mFrequencyBand_Stepsize = 100;
          break;
        }
        case REGIONAL_BAND_UNITED_STATES:
        {
          /*UNITED STATES : 88100 TO 107900 IN 200 KHZ STEPS*/
          mFMConfiguration.setRadioBand(FmReceiver.FM_US_BAND);
          mFMConfiguration.setChSpacing(FmReceiver.FM_CHSPACE_200_KHZ);
          mFMConfiguration.setEmphasis(FmReceiver.FM_DE_EMP75);
          mFMConfiguration.setRdsStd(FmReceiver.FM_RDS_STD_RBDS);
          mFMConfiguration.setLowerLimit(88100);
          mFMConfiguration.setUpperLimit(107900);
          mFrequencyBand_Stepsize = 200;
          break;
        }
        case REGIONAL_BAND_USER_DEFINED:
        {
          mFMConfiguration.setRadioBand(FmReceiver.FM_USER_DEFINED_BAND);
          mFMConfiguration.setChSpacing(mChanSpacing);
          mFMConfiguration.setEmphasis(FmReceiver.FM_DE_EMP75);
          mFMConfiguration.setRdsStd(FmReceiver.FM_RDS_STD_RDS);
          mFMConfiguration.setLowerLimit(mBandMinFreq);
          mFMConfiguration.setUpperLimit(mBandMaxFreq);
          if(mChanSpacing == 0) {
             mFrequencyBand_Stepsize = 200;
          }else if(mChanSpacing == 1) {
             mFrequencyBand_Stepsize = 100;
          }else {
             mFrequencyBand_Stepsize = 50;
          }
          break;
        }
        default:
        {
          Log.d(LOGTAG, "Invalid: countryCode: "+nCountryCode);
          nCountryCode=0;
        }
      }
      mCountry = nCountryCode;
      Log.d(LOGTAG, "=====================================================");
      Log.d(LOGTAG, "Country     :"+nCountryCode);
      Log.d(LOGTAG, "RadioBand   :"+ mFMConfiguration.getRadioBand());
      Log.d(LOGTAG, "Emphasis    :"+ mFMConfiguration.getEmphasis());
      Log.d(LOGTAG, "ChSpacing   :"+ mFMConfiguration.getChSpacing());
      Log.d(LOGTAG, "RdsStd      :"+ mFMConfiguration.getRdsStd());
      Log.d(LOGTAG, "LowerLimit  :"+ mFMConfiguration.getLowerLimit());
      Log.d(LOGTAG, "UpperLimit  :"+ mFMConfiguration.getUpperLimit());
      Log.d(LOGTAG, "=====================================================");
   }


   public static int getCountry() {
      return mCountry;
   }


   public static void setAudioOutputMode(boolean bStereo) {
      mAudioOutputMode = bStereo;
   }

   public static boolean getAudioOutputMode() {
      return mAudioOutputMode;
   }

   public static int getLastAudioMode() {
       return mLastAudioMode;
   }

   public static void setLastAudioMode(int audiomode) {
       mLastAudioMode = audiomode;
   }
   public static void setRecordDuration(int durationIndex) {

      Log.d(LOGTAG, "setRecordDuration "+durationIndex);
      switch( durationIndex ) {
      case 0: mRecordDuration = RECORD_DUR_INDEX_0_VAL; break;
      case 1: mRecordDuration = RECORD_DUR_INDEX_1_VAL; break;
      case 2: mRecordDuration = RECORD_DUR_INDEX_2_VAL; break;
      case 3: mRecordDuration = RECORD_DUR_INDEX_3_VAL; break;
      default:
        {
           Log.d(LOGTAG, "Invalid: durationIndex "+durationIndex);
        }

      }
      return;
   }

   public static int getRecordDuration() {
      return mRecordDuration;
   }

   public static void setAutoAFSwitch(boolean bAFAutoSwitch) {
      mAFAutoSwitch = bAFAutoSwitch;
   }

   public static boolean getAutoAFSwitch() {
      return mAFAutoSwitch;
   }
}

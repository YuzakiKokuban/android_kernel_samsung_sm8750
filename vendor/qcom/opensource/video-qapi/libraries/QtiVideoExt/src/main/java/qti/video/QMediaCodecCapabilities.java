/*
 **************************************************************************************************
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 **************************************************************************************************
 */

package qti.video;

import android.content.Context;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Encapsulates the capabilities of a given codec component. You can get all
 * vendor parameter names, descriptions and supported values.
 *
 * @version 0.1
 */
public final class QMediaCodecCapabilities {

  private static final String TAG = "QMediaCodecCapabilities";

  private final MediaCodecInfo info;
  private final String mimeType;
  private final Context context;
  private final String jsonFile;
  private HashMap<String, SupportedValues> paramRanges;

  private QMediaCodecCapabilities(String mime, MediaCodecInfo info, Context context) {
    this.context = context;
    this.info = info;
    this.mimeType = mime;
    this.paramRanges = new HashMap<>();

    // look for if a json asset file for this device exists
    String assetFile = null;
    // if context is null, assume non-legacy device --> in populateRangeMap (in createForCodec)
    // will verify if the non-legacy device is a supported device or not
    if (context != null) {
      String[] list;
      try {
        list = context.getAssets().list("");
        String board = Build.BOARD.toLowerCase(Locale.ROOT);
        String model = Build.SOC_MODEL.toLowerCase(Locale.ROOT);
        for (String file : list) {
          if (file.contains("QMediaCodecCapabilities")) {
            if (file.contains(board) || file.contains(model)) {
              // TODO: to test JUnit
              // legacy (uncomment following line of code)
              // non-legacy (comment following line of code)
              assetFile = file;
              break;
            }
          }
        }
      } catch (IOException e) {
        Log.e(TAG, "Failed to getAssets()");
      }
    }
    this.jsonFile = assetFile;
  }

  /**
   * Instantiate the QMediaCodecCapabilities for the given codec name and mime type combination.
   * * You can get a codec name via
   * <a href="https://developer.android.com/reference/android/media/MediaCodec#getName()">
   *   MediaCodec.getName</a> or
   * <a href="https://developer.android.com/reference/android/media/MediaCodecInfo#getName()">
   *   MediaCodecInfo.getName()</a>
   *
   * @param codecName underlying codec name or alias name
   * @param mime      media type
   * @return QMediaExtensions instance, null if the codec name and mime type combination is
   *     unsupported
   */
  public static QMediaCodecCapabilities createForCodec(String codecName, String mime,
                                                       Context context) {
    // check that the user is requesting a valid codec
    MediaCodecInfo info = verifyValidCodecAndMime(codecName, mime);
    if (info == null) {
      Log.e(TAG, "Not a valid codec/mime");
      return null;
    }

    QMediaCodecCapabilities codecCapabilities = new QMediaCodecCapabilities(mime, info, context);

    // try to query param ranges
    if (!codecCapabilities.populateRangeMap(null)) {
      Log.e(TAG, "Failed to populate range map for codec" + codecName);
      return null;
    }
    return codecCapabilities;
  }

  // TODO: make it public?
  static QMediaCodecCapabilities createForCodec(MediaCodec codec, String mime,
                                                       Context context) {
    // check that the user is requesting a valid codec
    String codecName = codec.getName();
    MediaCodecInfo info = verifyValidCodecAndMime(codecName, mime);
    if (info == null) {
      Log.e(TAG, "Not a valid codec/mime");
      return null;
    }

    QMediaCodecCapabilities codecCapabilities = new QMediaCodecCapabilities(mime, info, context);

    // try to query param ranges
    if (!codecCapabilities.populateRangeMap(codec)) {
      Log.e(TAG, "Failed to populate range map for codec" + codecName);
      return null;
    }
    return codecCapabilities;
  }

  /**
   * private helper method to verify the codec and profile-level.
   *
   * @return Boolean true if codec supports for HEVC codec and
   *     HEVCMAIN10 profile-level, false otherwise
   */
  private static boolean isHevcEncoderAnd10bitModeSupported(MediaCodec mediaCodec) {
    Log.d(TAG, "Codec type: " + mediaCodec.getName());
    MediaCodecInfo info = mediaCodec.getCodecInfo();
    MediaCodecInfo.CodecCapabilities cap =
        info.getCapabilitiesForType(MediaFormat.MIMETYPE_VIDEO_HEVC);
    MediaFormat format = new MediaFormat();
    format.setString(MediaFormat.KEY_MIME, MediaFormat.MIMETYPE_VIDEO_HEVC);
    format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.HEVCProfileMain10);
    if (cap.isFormatSupported(format)) {
      format.setInteger(MediaFormat.KEY_PROFILE,
          MediaCodecInfo.CodecProfileLevel.HEVCProfileMain10HDR10);
      if (cap.isFormatSupported(format)) {
        format.setInteger(MediaFormat.KEY_PROFILE,
            MediaCodecInfo.CodecProfileLevel.HEVCProfileMain10HDR10Plus);
        if (cap.isFormatSupported(format)) {
          Log.i(TAG, "isHEVCEncoderAnd10bitModesupported: true");
          return true;
        } else {
          Log.i(TAG, "isHEVCEncoderAnd10bitModesupported: false");
        }
      } else {
        Log.i(TAG, "isHEVCEncoderAnd10bitModesupported: false");
      }
    } else {
      Log.i(TAG, "isHEVCEncoderAnd10bitModesupported: false");
    }
    return false;
  }

  /**
   * helper method to check if the prosight mode is supported
   * Codec will be reset here.
   *
   * @return true if the prosight mode is supported, else false
   */
  public static boolean isProSightSupported(MediaCodec mediaCodec) {
    if (isHevcEncoderAnd10bitModeSupported(mediaCodec)) {
      String codecName = mediaCodec.getName();
      QMediaCodecCapabilities ext =
          QMediaCodecCapabilities.createForCodec(codecName, MediaFormat.MIMETYPE_VIDEO_HEVC, null);
      if (ext == null) {
        return false;
      }
      SupportedValues<Integer> range = ext.getParameterRangeInteger(
          QMediaExtensions.KEY_PROSIGHT_ENCODER_MODE);
      if (range == null) {
        return false;
      }
      int supportedRangeValue = QMediaExtensions.ProSightExtensionRange.PROSIGHT.getValue();
      if (range.values.contains(supportedRangeValue)) {
        Log.i(TAG, "isProSightSupported: " + "true(Vendor range supported)");
        return true;
      } else {
        List<String> vendorParams = mediaCodec.getSupportedVendorParameters();
        for (int i = 0; i < vendorParams.size(); i++) {
          Log.d(TAG, vendorParams.get(i));
        }
        if (vendorParams.contains(
            QMediaExtensions.KEY_PROSIGHT_ENCODER_MODE)) {
          Log.i(TAG, "isvendorextensionsupported: " + "true");
          Log.i(TAG, "isProSightSupported: " + "false");
        } else {
          Log.i(TAG, "isvendorextensionsupported: " + "false");
          Log.i(TAG, "isProSightSupported: " + "false");
        }
      }
    }
    return false;
  }

  /**
   * helper method to enable the prosight mode if supported.
   *
   * @return MediaFormat with prosight mode if the prosight mode is supported
   *     else MediaFormat without prosight mode
   */
  public static MediaFormat enableProSight(MediaFormat mediaFormat) {
    Log.d(TAG, "enableProSight API called");
    if (mediaFormat.containsKey(MediaFormat.KEY_MIME)
        && mediaFormat.containsKey(MediaFormat.KEY_PROFILE)) {
      String mime = mediaFormat.getString(MediaFormat.KEY_MIME);
      int profile = mediaFormat.getInteger(MediaFormat.KEY_PROFILE);
      Log.d(TAG, "Mime extracted from Application: " + mime);
      Log.d(TAG, "ProfilLevel extracted from Application: " + profile);
      if ((MediaFormat.MIMETYPE_VIDEO_HEVC.equals(mime))
          && (profile == MediaCodecInfo.CodecProfileLevel.HEVCProfileMain10
          || profile == MediaCodecInfo.CodecProfileLevel.HEVCProfileMain10HDR10
          || profile == MediaCodecInfo.CodecProfileLevel.HEVCProfileMain10HDR10Plus)) {
        final int kBitrate = 100000000;
        final String kColorFormat = "P010";
        final int kFramerate = 30;
        final int kCaptureRate = 30;
        final int kIframeInterval = 0;
        final int kEnableVendorExtension = 1;
        if (!mediaFormat.containsKey(MediaFormat.KEY_BIT_RATE)) {
          mediaFormat.setLong(MediaFormat.KEY_BIT_RATE, kBitrate);
        }
        if (!mediaFormat.containsKey(MediaFormat.KEY_COLOR_FORMAT)) {
          mediaFormat.setString(MediaFormat.KEY_COLOR_FORMAT, kColorFormat);
        }
        if (!mediaFormat.containsKey(MediaFormat.KEY_FRAME_RATE)) {
          mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, kFramerate);
        }
        if (!mediaFormat.containsKey(MediaFormat.KEY_CAPTURE_RATE)) {
          mediaFormat.setInteger(MediaFormat.KEY_CAPTURE_RATE, kCaptureRate);
        }
        mediaFormat.setInteger(MediaFormat.KEY_BITRATE_MODE,
            MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_VBR);
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, kIframeInterval);
        mediaFormat.setInteger(QMediaExtensions.KEY_PROSIGHT_ENCODER_MODE, kEnableVendorExtension);
        Log.i(TAG, "Setting the vendor extension to 1");
      }
    } else {
      Log.e(TAG, "mime/profile not supported");
    }
    return mediaFormat;
  }

  /**
   * private helper method to verify user input for making a codec is valid.
   *
   * @param codecName the input name of the codec (NOT the canon name)
   * @param mime      the input mime type of the codec
   * @return valid MediaCodecInfo for the codec if valid codecName and mime combination,
   *     null otherwise
   */
  private static MediaCodecInfo verifyValidCodecAndMime(String codecName, String mime) {
    MediaCodecInfo[] mediaCodecInfos =
        new MediaCodecList(MediaCodecList.ALL_CODECS).getCodecInfos();
    // check if this codec exists
    for (MediaCodecInfo codecInfo : mediaCodecInfos) {
      if (codecInfo.getName().equalsIgnoreCase(codecName)) {
        // verify mime type
        String[] types = codecInfo.getSupportedTypes();
        for (String type : types) {
          if (type.equalsIgnoreCase(mime)) {
            return codecInfo;
          }
        }
        break;
      }
    }
    return null;
  }

  /**
   * package scope method to get the assetfile (for junit/debugging).
   *
   * @return the name of the JSON asset file
   */
  String getAssetFile() {
    return this.jsonFile;
  }

  /**
   * helper method to check if the device is a (supported) legacy device.
   *
   * @return true if device is a supported legacy device (meaning that the json asset file exists)
   */
  private boolean isLegacyDevice() {
    return (jsonFile != null);
  }

  /**
   * Get the parameter's supported values range as an integer value range.
   *
   * @param parameterName name of the parameter to describe, typically one from
   *                      <a href="https://developer.android.com/reference/android/media/MediaCodec#getSupportedVendorParameters()">MediaCodec.getSupportedVendorParameters()</a>.
   * @return SupportedValues object that describes the supported range. null if
   *     unrecognized / not able to get range / parameter's value type is not integer.
   */
  public SupportedValues<Integer> getParameterRangeInteger(String parameterName) {
    if (getParameterType(parameterName) != MediaFormat.TYPE_INTEGER) {
      Log.e(TAG, "Parameter type mismatch (not an Integer parameter)");
      return null;
    }
    if (populateParameterRange(parameterName)) {
      @SuppressWarnings("unchecked") SupportedValues<Integer> range =
          paramRanges.get(parameterName);
      return range;
    } else {
      Log.e(TAG, "Failed to query range for parameter " + parameterName);
      return null;
    }
  }

  /**
   * Get the parameter's supported values range as an long value range.
   *
   * @param parameterName name of the parameter to describe, typically one from
   *                      <a href="https://developer.android.com/reference/android/media/MediaCodec#getSupportedVendorParameters()">MediaCodec.getSupportedVendorParameters()</a>.
   * @return SupportedValues object that describes the supported range. null if
   *     unrecognized / not able to get range / parameter's value type is not long.
   */
  public SupportedValues<Long> getParameterRangeLong(String parameterName) {
    if (getParameterType(parameterName) != MediaFormat.TYPE_LONG) {
      Log.e(TAG, "Parameter type mismatch (not a Long parameter)");
      return null;
    }
    if (populateParameterRange(parameterName)) {
      @SuppressWarnings("unchecked") SupportedValues<Long> range = paramRanges.get(parameterName);
      return range;
    } else {
      Log.e(TAG, "Failed to query range for parameter " + parameterName);
      return null;
    }
  }

  /**
   * Get the parameter's supported values range as an float value range.
   *
   * @param parameterName name of the parameter to describe, typically one from
   *                      <a href="https://developer.android.com/reference/android/media/MediaCodec#getSupportedVendorParameters()">MediaCodec.getSupportedVendorParameters()</a>.
   * @return SupportedValues object that describes the supported range. null if
   *     unrecognized / not able to get range / parameter's value type is not float.
   */
  public SupportedValues<Float> getParameterRangeFloat(String parameterName) {
    if (getParameterType(parameterName) != MediaFormat.TYPE_FLOAT) {
      Log.e(TAG, "Parameter type mismatch (not a Float parameter)");
      return null;
    }
    if (populateParameterRange(parameterName)) {
      @SuppressWarnings("unchecked") SupportedValues<Float> range = paramRanges.get(parameterName);
      return range;
    } else {
      Log.e(TAG, "Failed to query range for parameter " + parameterName);
      return null;
    }
  }

  /**
   * Get the parameter's supported values range as an string value range.
   *
   * @param parameterName name of the parameter to describe, typically one from
   *                      <a href="https://developer.android.com/reference/android/media/MediaCodec#getSupportedVendorParameters()">MediaCodec.getSupportedVendorParameters()</a>.
   * @return SupportedValues object that describes the supported range. null if
   *     unrecognized / not able to get range / parameter's value type is not string. For now,
   *     the type of returned {@link SupportedValues} is always {@link SupportedValues.Type#ANY}
   */
  public SupportedValues<Integer> getParameterRangeForString(String parameterName) {
    if (getParameterType(parameterName) != MediaFormat.TYPE_STRING) {
      Log.e(TAG, "Parameter type mismatch (not a String parameter)");
      return null;
    }
    if (populateParameterRange(parameterName)) {
      @SuppressWarnings("unchecked") SupportedValues<Integer> range =
          paramRanges.get(parameterName);
      return range;
    } else {
      Log.e(TAG, "Failed to query range for parameter " + parameterName);
      return null;
    }
  }

  /**
   * Get the parameter supported values range as an ByteBuffer value range.
   *
   * @param parameterName name of the parameter to describe, typically one from
   *                      <a href="https://developer.android.com/reference/android/media/MediaCodec#getSupportedVendorParameters()">MediaCodec.getSupportedVendorParameters()</a>.
   * @return SupportedValues object that describes the supported range. null if
   *     unrecognized / not able to get range / parameter's value type is not ByteBuffer. For now,
   *     the type of returned {@link SupportedValues} is always {@link SupportedValues.Type#ANY}
   */
  public SupportedValues<Integer> getParameterRangeForByteBuffer(String parameterName) {
    if (getParameterType(parameterName) != MediaFormat.TYPE_BYTE_BUFFER) {
      Log.e(TAG, "Parameter type mismatch (not a Byte Buffer parameter)");
      return null;
    }
    if (populateParameterRange(parameterName)) {
      @SuppressWarnings("unchecked") SupportedValues<Integer> range =
          paramRanges.get(parameterName);
      return range;
    } else {
      Log.e(TAG, "Failed to query range for parameter " + parameterName);
      return null;
    }
  }

  /**
   * helper method to do the actual param query.
   *
   * @param parameterName name string of "parameter.field" that is being queried
   * @return true if the parameter was found/successfully queried and false if fail
   */
  private boolean populateParameterRange(String parameterName) {
    // try to populate range map again if it is empty -- although it shouldn't be
    if (paramRanges == null || paramRanges.isEmpty()) {
      populateRangeMap(null);
    }
    SupportedValues<?> range = paramRanges.get(parameterName);
    if (range != null) {
      return true;
    } else {
      Log.e(TAG, "Failed to query parameter " + parameterName + " in asset file");
      return false;
    }
  }

  /**
   * Get parameter descriptors and supported values for all the supported vendor parameters.
   *
   * @return a immutable map mapping from
   *     <a href="https://developer.android.com/reference/android/media/MediaCodec.ParameterDescriptor">MediaCodec.getParameterDescriptor</a>
   *     objects to {@link SupportedValues} objects. If unsuccessful returns empty map.
   */
  public Map<String, SupportedValues> getSupportedParameterRanges() {
    // should not need to re-get param ranges if they have already been fetched
    // for this codec since they shouldn't change
    // because only fetches static range
    if (paramRanges == null || paramRanges.isEmpty()) {
      populateRangeMap(null);
      // TECHNICALLY IF THE FIRST POPULATE RANGE MAP FAILS THE QMEDIACODECCAPABILITY ISNT CREATED
      // SO IF MAP IS NULL OR EMPTY MAYBE JUST RETURN NULL INSTEAD???????
    }
    return Collections.unmodifiableMap(paramRanges);
  }

  private byte[] readAllBytes(InputStream inputStream) throws IOException {
    final int kBufferSize = 4096;
    List<byte[]> buffers = new ArrayList<>();
    int lastReadBytes = 0;
    int readBytes;
    do {
      byte[] buf = new byte[kBufferSize];
      readBytes = inputStream.read(buf, 0, kBufferSize);
      if (readBytes == -1) {
        break;
      } else {
        buffers.add(buf);
        lastReadBytes = readBytes;
      }
    } while (true);
    if (buffers.size() == 0) {
      return new byte[0];
    }
    final int total = kBufferSize * (buffers.size() - 1) + lastReadBytes;
    byte[] result = new byte[total];
    int offset = 0;
    int remaining = total;
    for (byte[] buf : buffers) {
      int sz = Math.min(buf.length, remaining);
      System.arraycopy(buf, 0, result, offset, sz);
      offset += sz;
      remaining -= sz;
    }
    return result;
  }

  /**
   * private helper method during creation of QMediaCodecCapabilities obj to query param ranges.
   *
   * @return true if param range map was properly populated, false otherwise
   */
  private boolean populateRangeMap(MediaCodec codec) {
    paramRanges = new HashMap<>();
    // populate the map
    String codecName = info.getName();
    String canonName = info.getCanonicalName();
    String fileName = jsonFile;
    if (isLegacyDevice()) {
      String json;
      try (InputStream inputStream = context.getAssets().open(fileName)) {
        json = new String(readAllBytes(inputStream), StandardCharsets.UTF_8);
      } catch (IOException e) {
        Log.e(TAG, "Failed to open asset file " + fileName + " as stream");
        return false;
      } catch (Exception e) {
        Log.e(TAG, "Failed to read asset file");
        return false;
      }
      JSONObject jsonObject;
      try {
        jsonObject = new JSONObject(json);
      } catch (JSONException e) {
        Log.e(TAG, "Failed to parse asset file to jsonObject");
        return false;
      }
      JSONObject codecs = jsonObject.optJSONObject("codecs.params");
      if (codecs == null) {
        Log.e(TAG, "Failed to get codecs from JSON file");
        return false;
      }
      JSONObject params = codecs.optJSONObject(canonName);
      if (params == null) {
        Log.e(TAG, "Codec " + canonName + " not found in asset file!");
        return false;
      }
      if (parseNestedParamRangeFromJson(null, params)) {
        return true;
      } else {
        Log.e(TAG, "Failed to parse params json from asset file");
        return false;
      }
    } else {
      // not legacy device
      // setup MediaFormat
      // the max size out of all the min sizes for each of the codecs
      final int kWidth = 512;
      final int kHeight = 512;
      final int kBitrate = 2000000;
      final int kIframeInterval = 2;
      final int kFramerate = 30;

      MediaFormat format = MediaFormat.createVideoFormat(mimeType, kWidth, kHeight);
      // set framerate later because in some build versions format must not contain frame rate
      format.setInteger(MediaFormat.KEY_BIT_RATE, kBitrate);
      format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, kIframeInterval);
      int flag = 0;
      if (codecName.contains("encoder")) {
        // if encoder
        flag = MediaCodec.CONFIGURE_FLAG_ENCODE;
      }

      MediaCodec mediaCodec;
      if (codec != null) {
        mediaCodec = codec;
        // reset state
        mediaCodec.reset();
      } else {
        // create codec if no existing codec is provided
        try {
          Log.v(TAG, "Creating Codec " + codecName);
          mediaCodec = MediaCodec.createByCodecName(codecName);
        } catch (Exception e) {
          Log.e(TAG, "Failed to create codec");
          return false;
        }
      }

      // check if this device is supported (since it's not a legacy device)
      List<String> vendorParams = mediaCodec.getSupportedVendorParameters();
      final String c2CapsQueryInput_Key = "vendor.qti-ext-caps-query-input.value";
      final String c2CapsQueryOutput_Key = "vendor.qti-ext-caps-query-output.value";
      if (!vendorParams.contains(c2CapsQueryInput_Key)
          || !vendorParams.contains(c2CapsQueryOutput_Key)) {
        Log.e(TAG, "Not a supported device");
        return false;
      }
      // reset the codec to put it back to uninitialized state
      mediaCodec.reset();
      // subscribe to query result param
      List<String> queryParams = new ArrayList<>();
      queryParams.add(c2CapsQueryOutput_Key);
      mediaCodec.subscribeToVendorParameters(queryParams);

      if (canonName.contains("encoder")) {
        // codec is encoder
        flag = MediaCodec.CONFIGURE_FLAG_ENCODE;
      }
      format.setInteger(MediaFormat.KEY_FRAME_RATE, kFramerate);
      format.setString(c2CapsQueryInput_Key, "[\"vendor.*\"]");
      try {
        mediaCodec.configure(format, null, null, flag);
      } catch (Exception e) {
        e.printStackTrace();
        Log.e(TAG, "Fetch param range failed; failed to configure (codec in released state)");
        return false;
      }
      MediaFormat outFormat = mediaCodec.getOutputFormat();
      if (codec == null) {
        // if no codec was provided, then this is our private instance; release it
        mediaCodec.release();
      } else {
        // this codec instance was provided to us; reset state
        mediaCodec.reset();
      }
      if (!outFormat.containsKey(c2CapsQueryOutput_Key)) {
        Log.e(TAG, "Query for param ranges FAILED");
        return false;
      }
      // parse json result
      String json = outFormat.getString(c2CapsQueryOutput_Key);
      JSONObject params;
      try {
        params = new JSONObject(json);
      } catch (JSONException e) {
        e.printStackTrace();
        return false;
      }
      if (parseNestedParamRangeFromJson(null, params)) {
        return true;
      } else {
        Log.e(TAG, "Failed to parse params json from full query");
        return false;
      }
    }
  }

  private boolean parseNestedParamRangeFromJson(String parentName, JSONObject params) {
    if (params == null) {
      Log.e(TAG, "Null JSON param result");
      return false;
    }
    JSONArray paramNames = params.names();
    if (paramNames != null) {
      for (int i = 0; i < paramNames.length(); i++) {
        String paramName = paramNames.optString(i);
        if (paramName == null) {
          // this should never happen
          Log.e(TAG, "Failed to get paramName from paramNames");
          return false;
        }
        JSONObject internal = params.optJSONObject(paramName);
        if (internal == null) {
          // this should never happen
          Log.e(TAG, "Failed to get param from params");
          return false;
        }
        String fullName = (parentName == null) ? paramName : parentName + "." + paramName;
        if (!internal.has("values") || !internal.has("valuestype") || !internal.has("datatype")) {
          if (!parseNestedParamRangeFromJson(fullName, internal)) {
            return false;
          }
        } else {
          // else make the SupportedValue obj and add to map
          String typestr = internal.optString("valuestype");
          String datatypestr = internal.optString("datatype");
          JSONArray arr = internal.optJSONArray("values");
          if (typestr.equals("") || datatypestr.equals("") || arr == null) {
            // this should never happen
            Log.e(TAG, "Failed to get range data for param " + fullName);
            return false;
          }
          SupportedValues.Type type = getParamRangeType(typestr);
          SupportedValues.DataType datatype = getParamDataType(datatypestr);
          ValueRange<?> range;
          List<?> values;
          if (type == SupportedValues.Type.RANGE) {
            range = getRangeFromJson(datatypestr, arr);
            values = null;
          } else if (type == SupportedValues.Type.EMPTY) {
            // make both range and values null
            range = null;
            values = null;
          } else if (type == SupportedValues.Type.ANY) {
            if (checkForUnsignedRange(datatypestr)) {
              type = SupportedValues.Type.RANGE;
              range = getRangeForAny(datatypestr);
            } else {
              // make range null
              range = null;
            }
            values = null;
          } else {
            // for all other types (values/flags)
            values = getValuesFromJson(datatypestr, arr);
            range = null;
          }
          @SuppressWarnings("unchecked") SupportedValues<?> supportedValues =
              new SupportedValues(type, datatype, range, values);
          paramRanges.put(fullName, supportedValues);
        }
      }
      return true;
    } else {
      Log.e(TAG, "Failed to get params from codec");
      return false;
    }
  }

  private SupportedValues.DataType getParamDataType(String datatypeStr) {
    switch (datatypeStr.toLowerCase(Locale.ROOT)) {
      case "int32":
      case "uint32":
      case "cntr32":
        return SupportedValues.DataType.INTEGER;
      case "string":
        return SupportedValues.DataType.STRING;
      case "blob":
        return SupportedValues.DataType.BYTE_BUFFER;
      case "int64":
      case "uint64":
      case "cntr64":
        return SupportedValues.DataType.LONG;
      case "float":
        return SupportedValues.DataType.FLOAT;
      case "struct_flag":
      default:
        return SupportedValues.DataType.NULL;
    }
  }

  private SupportedValues.Type getParamRangeType(String typeStr) {
    switch (typeStr.toLowerCase(Locale.ROOT)) {
      case "any":
        return SupportedValues.Type.ANY;
      case "flags":
        return SupportedValues.Type.FLAGS;
      case "values":
        return SupportedValues.Type.VALUES;
      case "range":
        return SupportedValues.Type.RANGE;
      default:
        return SupportedValues.Type.EMPTY;
    }
  }

  private List<?> getValuesFromJson(String datatypeStr, JSONArray array) {
    if (array == null) {
      Log.e(TAG, "Improperly formatted JSON values result");
      return null;
    }
    try {
      if (datatypeStr.equalsIgnoreCase("int32")
          || datatypeStr.equalsIgnoreCase("uint32")
          || datatypeStr.equalsIgnoreCase("cntr32")
          || datatypeStr.equalsIgnoreCase("blob")
          || datatypeStr.equalsIgnoreCase("string")) {
        List<Integer> intList = new ArrayList<>();
        for (int i = 0; i < array.length(); i++) {
          int value = array.getInt(i);
          intList.add(value);
        }
        return intList;
      } else if (datatypeStr.equalsIgnoreCase("int64")
          || datatypeStr.equalsIgnoreCase("uint64")
          || datatypeStr.equalsIgnoreCase("cntr64")) {
        List<Long> longList = new ArrayList<>();
        for (int i = 0; i < array.length(); i++) {
          long value = array.getLong(i);
          longList.add(value);
        }
        return longList;
      } else if (datatypeStr.equalsIgnoreCase("float")) {
        List<Float> floatList = new ArrayList<>();
        for (int i = 0; i < array.length(); i++) {
          float value = (float) array.getDouble(i);
          floatList.add(value);
        }
        return floatList;
      } else if (datatypeStr.equalsIgnoreCase("struct_flag")) {
        return null;
      } else {
        return null;
      }
    } catch (JSONException e) {
      e.printStackTrace();
    }
    return null;
  }

  private boolean checkForUnsignedRange(String datatypeStr) {
    boolean isUnsigned = false;
    switch (datatypeStr.toLowerCase(Locale.ROOT)) {
      case "uint32":
      case "cntr32":
      case "uint64":
      case "cntr64":
      case "blob":
      case "string":
        isUnsigned = true;
        break;
      default:
        break;
    }
    return isUnsigned;
  }

  private ValueRange<?> getRangeForAny(String datatypeStr) {
    if (datatypeStr.equalsIgnoreCase("uint32") || datatypeStr.equalsIgnoreCase("cntr32")) {
      int min = 0;
      int max = Integer.MAX_VALUE;
      int step = 1;
      int num = 1;
      int denom = 1;
      return new ValueRange<>(min, max, step, num, denom);
    } else if (datatypeStr.equalsIgnoreCase("string") || datatypeStr.equalsIgnoreCase("blob")) {
      int min = 0;
      int max = 255;
      int step = 1;
      int num = 1;
      int denom = 1;
      return new ValueRange<>(min, max, step, num, denom);
    } else if (datatypeStr.equalsIgnoreCase("uint64") || datatypeStr.equalsIgnoreCase("cntr64")) {
      // this doesnt exactly cover the full range for uint64/cntr64 but
      // use long to match types used in Android MediaCodec API
      long min = 0;
      long max = Long.MAX_VALUE;
      long step = 1;
      long num = 1;
      long denom = 1;
      return new ValueRange<>(min, max, step, num, denom);
    } else if (datatypeStr.equalsIgnoreCase("int32")) {
      int min = Integer.MIN_VALUE;
      int max = Integer.MAX_VALUE;
      int step = 1;
      int num = 1;
      int denom = 1;
      return new ValueRange<>(min, max, step, num, denom);
    } else if (datatypeStr.equalsIgnoreCase("int64")) {
      long min = Long.MIN_VALUE;
      long max = Long.MAX_VALUE;
      long step = 1;
      long num = 1;
      long denom = 1;
      return new ValueRange<>(min, max, step, num, denom);
    } else if (datatypeStr.equalsIgnoreCase("float")) {
      float min = Float.MIN_VALUE;
      float max = Float.MAX_VALUE;
      float step = 1;
      float num = 1;
      float denom = 1;
      return new ValueRange<>(min, max, step, num, denom);
    } else {
      return null;
    }
  }

  /**
   * private helper method to parse JSON range result of RANGE type to ValueRange.
   *
   * @param datatypeStr string parsed from json representing the param datatype
   * @param array       the JSONArray holding range values parsed from the json
   * @return ValueRange object of the parameter field range
   */
  private ValueRange<?> getRangeFromJson(String datatypeStr, JSONArray array) {
    // length check to verify the range array is properly formatted
    if (array == null || array.length() != 5) {
      Log.e(TAG, "Improperly formatted JSON range result");
      return null;
    }
    try {
      if (datatypeStr.equalsIgnoreCase("int32")
          || datatypeStr.equalsIgnoreCase("uint32")
          || datatypeStr.equalsIgnoreCase("cntr32")
          || datatypeStr.equalsIgnoreCase("string")
          || datatypeStr.equalsIgnoreCase("blob")) {
        int min = array.getInt(0);
        int max = array.getInt(1);
        int step = array.getInt(2);
        int num = array.getInt(3);
        int denom = array.getInt(4);
        return new ValueRange<>(min, max, step, num, denom);
      } else if (datatypeStr.equalsIgnoreCase("int64")) {
        long min = array.getLong(0);
        long max = array.getLong(1);
        long step = array.getLong(2);
        long num = array.getLong(3);
        long denom = array.getLong(4);
        return new ValueRange<>(min, max, step, num, denom);
      } else if (datatypeStr.equalsIgnoreCase("uint64") || datatypeStr.equalsIgnoreCase("cntr64")) {
        long min = array.getLong(0);
        long max = array.getLong(1);
        long step = array.getLong(2);
        long num = array.getLong(3);
        long denom = array.getLong(4);
        return new ValueRange<>(min, max, step, num, denom);
      } else if (datatypeStr.equalsIgnoreCase("float")) {
        float min = (float) array.getDouble(0);
        float max = (float) array.getDouble(1);
        float step = (float) array.getDouble(2);
        float num = (float) array.getDouble(3);
        float denom = (float) array.getDouble(4);
        return new ValueRange<>(min, max, step, num, denom);
      } else if (datatypeStr.equalsIgnoreCase("struct_flag")) {
        return null;
      } else {
        return null;
      }
    } catch (JSONException e) {
      e.printStackTrace();
    }
    return null;
  }

  /**
   * Get the parameter's type; used for type checking for getParameterRangeXXX().
   *
   * @param paramName name of the parameter to describe, typically one from
   *                  <a href="https://developer.android.com/reference/android/media/MediaCodec#getSupportedVendorParameters()">MediaCodec.getSupportedVendorParameters()</a>.
   *                  This value cannot be null.
   * @return int type of the parameter
   *     value is MediaFormat.TYPE_NULL, MediaFormat.TYPE_INTEGER, MediaFormat.TYPE_LONG,
   *     MediaFormat.TYPE_FLOAT, MediaFormat.TYPE_STRING, or MediaFormat.TYPE_BYTE_BUFFER,
   *     returns MediaFormat.TYPE_NULL if unable to get descriptor for the parameter name.
   */
  private int getParameterType(String paramName) {
    if (paramRanges == null || paramRanges.isEmpty()) {
      // should not happen
      populateRangeMap(null);
    }
    SupportedValues<?> range = paramRanges.get(paramName);
    if (range != null) {
      switch (range.getDataType()) {
        case INTEGER:            // for int32/uint32/cntr32
          return MediaFormat.TYPE_INTEGER;
        case LONG:               // for int64/uint64/cntr64
          return MediaFormat.TYPE_LONG;
        case FLOAT:              // for float
          return MediaFormat.TYPE_FLOAT;
        case STRING:             // for string
          return MediaFormat.TYPE_STRING;
        case BYTE_BUFFER:        // for blob
          return MediaFormat.TYPE_BYTE_BUFFER;
        case NULL:
        default:
          return MediaFormat.TYPE_NULL;
      }
    } else {
      // if it's null, then the param isn't supported; return NULL
      Log.e(TAG, "Param " + paramName + " not supported");
      return MediaFormat.TYPE_NULL;
    }
  }

  /**
   * Range specifier for supported numeric value. Used if type is RANGE.
   *
   * <p>If step is 0 and num and denom are both 1, the supported values are any
   * value, for which
   * min <= value <= max.
   *
   * <p>Otherwise, the range represents a geometric/arithmetic/multiply-accumulate
   * series, where
   * successive supported values can be derived from previous values (starting at
   * min), using the
   * following formula:
   * v[0] = min
   * v[i] = v[i-1] * num / denom + step for i >= 1, while min < v[i] <= max.
   */
  public static class ValueRange<T> {

    private final T min;
    private final T max;
    private final T step;
    private final T num;
    private final T denom;

    ValueRange(T min, T max, T step, T num, T denom) {
      this.min = min;
      this.max = max;
      this.step = step;
      this.num = num;
      this.denom = denom;
    }

    /**
     * Get lower end of the range.
     *
     * @return Lower end of the range (inclusive).
     */
    public T getMin() {
      return min;
    }

    /**
     * Get Upper end of the range.
     *
     * @return Upper end of the range (inclusive).
     */
    public T getMax() {
      return max;
    }

    /**
     * Get the step of the range.
     *
     * @return The non-homogeneous term in the recurrence relation.
     */
    public T getStep() {
      return step;
    }

    /**
     * Get the numerator of the scale coefficient in the recurrence relation.
     *
     * @return The numerator of the scale coefficient in the recurrence relation.
     */
    public T getNum() {
      return num;
    }

    /**
     * Get the denominator of the scale coefficient in the recurrence relation.
     *
     * @return The denominator of the scale coefficient in the recurrence relation.
     */
    public T getDenom() {
      return denom;
    }
  }

  /**
   * Generic supported values for a field.
   *
   * <p>This can be either a range or a set of values. The range can be a simple
   * range, an arithmetic,
   * geometric or multiply-accumulate series with a clear minimum and maximum
   * value. Values can
   * be discrete values, or can optionally represent flags to be or-ed.
   *
   * <p>\note Do not use flags to represent bitfields. Use individual values or
   * separate fields instead.
   */
  public static class SupportedValues<T> {

    private final Type type;
    private final DataType datatype;
    private final ValueRange<T> range;
    private final List<T> values;

    SupportedValues(Type type, DataType datatype, ValueRange<T> range, List<T> values) {
      this.type = type;
      this.datatype = datatype;
      this.range = range;
      this.values = values;
    }

    /**
     * Get the type of the supported values.
     *
     * @return type of value for this field.
     *     Return value can only be one of EMPTY/RANGE/VALUES/FLAGS/ANY..
     */
    public Type getType() {
      return type;
    }

    /**
     * Get the data type of the supported values.
     *
     * @return datatype (Java type, matches MediaCodec type) of value for this field.
     *     return value can only be one of INTEGER/LONG/FLOAT/STRING/BYTE_BUFFER/NULL
     */
    public DataType getDataType() {
      return datatype;
    }

    /**
     * Get supported value range.
     *
     * @return supported value range if type is TYPE_RANGE, otherwise null.
     */
    public ValueRange<T> getRange() {
      if (getType() != Type.RANGE) {
        return null;
      }
      return range;
    }

    /**
     * Get a list of supported values.
     *
     * @return an immutable list of supported values if type is TYPE_VALUES, otherwise null.
     */
    public List<T> getValues() {
      if (getType() != Type.VALUES) {
        return null;
      }
      return values;
    }

    /**
     * Get supported flags.
     *
     * @return an immutable list of supported flags if type is TYPE_FLAGS, otherwise null.
     */
    public List<T> getFlags() {
      if (getType() != Type.FLAGS) {
        return null;
      }
      return values;
    }

    /**
     * Type of the supported values.
     */
    public enum Type {
      EMPTY,              // no supported values
      RANGE,              // a numeric range that can be continuous or discrete
      VALUES,             // a list of values
      FLAGS,              // a list of flags that can be OR-ed
      ANY                 // any value
    }

    /**
     * DataType enum denotes the type of data used by the parameter field.
     *
     * <p></p>The DataType matches the types returned from
     * <a href="https://developer.android.com/reference/android/media/MediaCodec.ParameterDescriptor#getType()">MediaCodec.ParameterDescriptor.getType()</a>
     */
    public enum DataType {
      NULL,               // for unknown or struct fields
      INTEGER,            // for int32/uint32/cntr32
      LONG,               // for int64/uint64/cntr64
      FLOAT,              // for float
      STRING,             // for string
      BYTE_BUFFER         // for blob
    }
  }
}

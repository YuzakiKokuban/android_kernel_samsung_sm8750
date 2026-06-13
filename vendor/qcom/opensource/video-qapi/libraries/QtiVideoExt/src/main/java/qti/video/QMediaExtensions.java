/*
 **************************************************************************************************
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 **************************************************************************************************
 */

package qti.video;

/**
 * QTI_MEDIACODEC_EXTENSION_VERSION 0.3
 * <p>
 * QTI Mediacodec extension APIs encapsulates the video extensions available for Snapdragon devices.
 * These extensions can be set on
 * <a href="https://developer.android.com/reference/android/media/MediaCodec">MediaCodec</a> object via
 * <a href="https://developer.android.com/reference/android/media/MediaCodec#configure(android.media.MediaFormat,%20android.view.Surface,%20android.media.MediaCrypto,%20int)">configure(..)</a> or
 * <a href="https://developer.android.com/reference/android/media/MediaCodec#setParameters(android.os.Bundle)">setParameters(..)</a> api.
 * <p>
 * <p>
 * The format of the media data is specified as key/value pairs. Keys are strings. Values can
 * be integer, long, float, String or ByteBuffer.
 * <p>
 * All the extensions may not be supported on a device. it is imperative that the applications
 * check the support of any extension by querying
 * <a href=https://developer.android.com/reference/android/media/MediaCodec#getSupportedVendorParameters()>MediaCodec.getSupportedVendorParameters(..)</a> api
 * <p>
 * <p>
 * <p>
 * List of QTI MedicaCodec extension APIs:
 * <table>
 * <tr><th>Name</th><th>Value Type</th><th>Description</th></tr>
 * <tr><td>{@link #KEY_INIT_QP_I_FRAME_ENABLE}</td><td>Integer</td><td>Enable override of initial
 * QP for I frame</td></tr>
 * <tr><td>{@link #KEY_INIT_QP_P_FRAME_ENABLE}</td><td>Integer</td><td>Enable override of initial
 * QP for P frame</td></tr>
 * <tr><td>{@link #KEY_INIT_QP_B_FRAME_ENABLE}</td><td>Integer</td><td>Enable override of initial
 * QP for B frame</td></tr>
 * <tr><td>{@link #KEY_INIT_QP_I_FRAME}</td><td>Integer</td><td>Initial QP value to be used for the I frame.
 * Initial QP must be enabled by setting {@link #KEY_INIT_QP_I_FRAME_ENABLE}</td></tr>
 * <tr><td>{@link #KEY_INIT_QP_P_FRAME}</td><td>Integer</td><td>Initial QP value to be used for the P frame.
 * Initial QP must be enabled by setting {@link #KEY_INIT_QP_P_FRAME_ENABLE}</td></tr>
 * <tr><td>{@link #KEY_INIT_QP_B_FRAME}</td><td>Integer</td><td>Initial QP value to be used for the B frame.
 * Initial QP must be enabled by setting {@link #KEY_INIT_QP_B_FRAME_ENABLE}</td></tr>
 * <tr><td>{@link #KEY_ROI_INFO_TYPE}</td><td>String</td><td>Region Of Interest payload type</td></tr>
 * <tr><td>{@link #KEY_ROI_RECT_INFO}</td><td>String</td><td>Region Of Interest payload specified with rectangles</td></tr>
 * <tr><td>{@link #KEY_ROI_RECT_INFO_EXT}</td><td>String</td><td>Extended ROI payload to specify additional rectangles</td></tr>
 * <tr><td>{@link #KEY_ROI_INFO_TIMESTAMP}</td><td>Integer</td><td>Timestamp of the frame to associate the ROI with</td></tr>
 * <tr><td>{@link #KEY_ADV_QP_BITRATE_MODE}</td><td>Integer</td><td>Enable application to control the rate control instead of by the encoder.
 * <tr><td>{@link #KEY_ADV_QP_FRAME_QP_VALUE}</td><td>Integer</td><td>QP value to be applied for compressing the next frame.
 * Advance QP must be enabled by setting {@link #KEY_ADV_QP_BITRATE_MODE} to 0</td></tr>
 * <tr><td>{@link #KEY_ER_RESYNC_MARKER_SIZE}</td><td>Integer</td><td>Resync Marker size in bits.
 * <tr><td>{@link #KEY_ER_SLICE_SPACING_SIZE}</td><td>Integer</td><td> Slice spacing size in Macroblocks.
 * <tr><td>{@link #KEY_ER_LTR_MAX_FRAMES}</td><td>Integer</td><td>Maximum number of LTR frames (slots) the encoder can store at any given time.
 * <tr><td>{@link #KEY_ER_LTR_MARK_FRAME}</td><td>Integer</td><td>Index of hte slot to store the LRT frame.
 * <tr><td>{@link #KEY_ER_LTR_USE_FRAME}</td><td>Integer</td><td>Slot index of a previously stored LTR frame to be used as reference.
 * <tr><td>{@link #KEY_ER_LTR_RESPONSE}</td><td>Integer</td><td> Response from encoder to a mark or use operation.
 * </table>
 * <br>
 * Extensions are optional and are applied based on best-effort.
 * <br /><br />
 */

public final class QMediaExtensions {

  /**
   * A key to enable override of default QP value used by the encoder for I frames.
   * <p>
   * The associated value is an integer:
   * <table>
   * <tr><th>Value</th><th>Description</th></tr>
   * <tr><td>1</td><td>enable initial QP override for I frames</td></tr>
   * <tr><td>0</td><td>disable initial QP override for I frames</td></tr>
   * </table>
   * <p>
   * Enabling I frame QP override will allow setting the initial QP value for I frames via {@link #KEY_INIT_QP_I_FRAME}
   * <p>
   * This key is accepted via MediaCodec#configure() before the encoder is started
   */
  public static final String KEY_INIT_QP_I_FRAME_ENABLE =
      "vendor.qti-ext-enc-initial-qp.qp-i-enable";

  /**
   * A key to enable override of default QP value used by the encoder for P frames.
   * <p>
   * The associated value is an integer:
   * <table>
   * <tr><th>Value</th><th>Description</th></tr>
   * <tr><td>1</td><td>enable initial QP override for P frames</td></tr>
   * <tr><td>0</td><td>disable initial QP override for P frames</td></tr>
   * </table>
   * <p>
   * Enabling P frame QP override will allow setting the inital QP value for P frames via {@link #KEY_INIT_QP_P_FRAME}
   * <p>
   * This key is accepted via MediaCodec#configure() before the encoder is started
   * <p>
   */
  public static final String KEY_INIT_QP_P_FRAME_ENABLE =
      "vendor.qti-ext-enc-initial-qp.qp-p-enable";

  /**
   * A key to enable override of default QP value used by the encoder for B frames.
   * <p>
   * The associated value is an integer:
   * <p>
   * <table>
   * <tr><th>Value</th><th>Description</th></tr>
   * <tr><td>1</td><td>enable initial QP override for B frames</td></tr>
   * <tr><td>0</td><td>disable initial QP override for B frames</td></tr>
   * </table>
   * Enabling B frame QP override will allow setting the initial QP value for B frames via {@link #KEY_INIT_QP_B_FRAME}
   * <p>
   * This key is accepted via MediaCodec#configure() before the encoder is started
   */
  public static final String KEY_INIT_QP_B_FRAME_ENABLE =
      "vendor.qti-ext-enc-initial-qp.qp-b-enable";

  /**
   * A key to set override for the initial QP value used by the encoder for I frames.
   * <p>
   * The associated value is an integer
   * <table>
   * <tr><th>Codec</th><th>Valid QP range</th></tr>
   * <tr><td>AVC</td><td>1 - 51</td></tr>
   * <tr><td>HEVC</td><td>1 - 51</td></tr>
   * </table>
   * I frame QP override must be enabled via {@link #KEY_INIT_QP_I_FRAME_ENABLE} to set this override
   * <p>
   * This key is accepted via MediaCodec#configure() before the encoder is started
   */
  public static final String KEY_INIT_QP_I_FRAME = "vendor.qti-ext-enc-initial-qp.qp-i";

  /**
   * A key to set override for the initial QP value used by the encoder for P frames.
   * <p>
   * The associated value is an integer
   * <table>
   * <tr><th>Codec</th><th>Valid QP range</th></tr>
   * <tr><td>AVC</td><td>1 - 51</td></tr>
   * <tr><td>HEVC</td><td>1 - 51</td></tr>
   * </table>
   * P frame QP override must be enabled via {@link #KEY_INIT_QP_P_FRAME_ENABLE} to set this override
   * <p>
   * This key is accepted via MediaCodec#configure() before the encoder is started
   */
  public static final String KEY_INIT_QP_P_FRAME = "vendor.qti-ext-enc-initial-qp.qp-p";

  /**
   * A key to set override for the initial QP value used by the encoder for B frames.
   * <p>
   * The associated value is an integer
   * <table>
   * <tr><th>Codec</th><th>Valid QP range</th></tr>
   * <tr><td>AVC</td><td>1 - 51</td></tr>
   * <tr><td>HEVC</td><td>1 - 51</td></tr>
   * </table>
   * B frame QP override must be enabled via {@link #KEY_INIT_QP_B_FRAME_ENABLE} to set this override
   * <p>
   * This key is accepted via MediaCodec#configure() before the encoder is started
   */
  public static final String KEY_INIT_QP_B_FRAME = "vendor.qti-ext-enc-initial-qp.qp-b";

  /**
   * A key to specify the type of Region Of Interest (ROI) payload.
   * <p>
   * The associated value is string. Following types are supported
   * <table>
   * <tr><th>Value</th><th>Description</th></tr>
   * <tr><td>"rect"</td><td>ROI is specified as <b>"Rectangles"</b>. Must use {@link #KEY_ROI_RECT_INFO} to configure ROI</td></tr>
   * </table>
   * <p>
   * This key is accepted via MediaCodec#configure() before the encoder is started
   */
  public static final String KEY_ROI_INFO_TYPE = "vendor.qti-ext-enc-roiinfo.type";

  /**
   * A key to configure Region Of Interest (ROI) with a set of rectangles.
   * <p>
   * This configuration can be applied per-frame to override the QP decision of the encoder within
   * certain regions of interest to keep or enhance the quality.
   * <p>
   * The region of interest is specified as a set of rectangles. Each rectangle is associated
   * with a QP bias. The QP bias is applied on the encoder-determined QP to determine the final
   * QP per macroblock. A negative value of QP bias will reduce the QP and hence improve quality.
   * <p>
   * The associated value for this key is a string that contains up to 5 rectangles along with the
   * QP-bias encoded with the following syntax (R1, R2 .. are rectangles, up to 5)
   * <p><b>
   * “R1_top,R1_left-R1_bottom,R1_right=QP_1;R2_top,R2_left-R2_bottom,R2_right=QP_2;.."
   * </b>
   * Below are a couple of examples
   * <table>
   * <tr><th>Rectangle info</th><th>Description</th></tr>
   * <tr><td>“100,100-400,400=-6;"</td><td>Specifies 1 rectangle [(100,100) (400,400)] with bias = -6</td></tr>
   * <tr><td>“100,100-400,400=-6;200,200-300,300=-4"</td><td>Specifies 2 rectangles. [(100,100) (400,400)] with bias = -6 and [(200,200) (400,400)] with bias = -4</td></tr>
   * </table>
   * <p>
   * This key is accepted via MediaCodec#setParameters() with every frame
   *
   * @apiNote It is important to associate the timestamp with KEY_ROI_INFO_TIMESTAMP
   * @apiNote {@link #KEY_ROI_RECT_INFO_EXT} can be optionally used if the ROI cannot be specified by 5 rectangular regions
   */
  public static final String KEY_ROI_RECT_INFO = "vendor.qti-ext-enc-roiinfo.rect-payload";

  /**
   * This key is an extension to {@link #KEY_ROI_RECT_INFO} to configure additional rectangles.
   * <p>
   * {@link #KEY_ROI_RECT_INFO} supports up to 5 rectangular regions. This key allows augmenting the
   * rectangles with 5 more (total of 10 rectangles)
   * <p>
   * This key is accepted via MediaCodec#setParameters() with every frame
   *
   * @apiNote This key cannot be set without setting {@link #KEY_ROI_RECT_INFO}
   */
  public static final String KEY_ROI_RECT_INFO_EXT = "vendor.qti-ext-enc-roiinfo.rect-payload-ext";

  /**
   * A key to specify the timestamp (in microseconds) of the frame that the ROI region (specified with {@link #KEY_ROI_RECT_INFO}) corresponds to.
   * <p>
   * The associated value is an integer indicating the corresponding frame timestamp in microseconds
   * <p>
   * This parameter is necessary if the applications queued the frames to encoder via a input Surface. Since the submission of
   * frame to encoder (via eglSwapBuffers) is not synchronized with MediaCodec.setParameters(), the specified ROI can go out
   * of sync with the corresponding frame. Attaching timestamp with the ROI enables the encoder to synchronize ROI data
   * with the frame.
   * This key is accepted via MediaCodec#setParameters() with every frame along with the {@link #KEY_ROI_RECT_INFO} parameter
   *
   * @apiNote It is important to set the ROI bundle (ROI_RECT_INFO + ROI_INFO_TIMESTAMP) ahead of queuing the frame to ensure the ROI
   * data is not missed
   */
  public static final String KEY_ROI_INFO_TIMESTAMP = "vendor.qti-ext-enc-roiinfo.type";

  /**
   * A key to enable override of default QP value used by the encoder for I frames.
   * <p>
   * The associated value is an integer:
   * <table>
   * <tr><th>Value</th><th>Description</th></tr>
   * <tr><td>1</td><td>enable initial QP override for I frames</td></tr>
   * <tr><td>0</td><td>disable initial QP override for I frames</td></tr>
   * </table>
   * <p>
   * Enabling I frame QP override will allow setting the initial QP value for I frames via {@link #KEY_INIT_QP_I_FRAME}
   * <p>
   * This key is accepted via MediaCodec#configure() before the encoder is started
   */
  public static final String KEY_ADV_QP_BITRATE_MODE = "vendor.qti-ext-enc-bitrate-mode.value";

  /**
   * A key to specify the QP value to be applied for compressing the next frame.
   * <p>
   * The associated value is an integer
   *
   * <p>
   * This value has to be configured dynamically per frame
   * This key is accepted via MediaCodec#SetParameters() before the encoder is started
   */
  public static final String KEY_ADV_QP_FRAME_QP_VALUE = "vendor.qti-ext-enc-frame-qp.value";


  /**
   * A key to specify the resync marker size in bits.
   * <p>
   * The associated value is an integer
   *
   * <p>
   * This key is accepted via MediaCodec#configure() before the encoder is started
   */
  public static final String KEY_ER_RESYNC_MARKER_SIZE =
      "vendor.qti-ext-enc-error-correction.resync-marker-spacing.bits";

  /**
   * A key to specify the slice spacing in Macbroblocks.
   * <p>
   * The associated value is an integer
   *
   * <p>
   * This key is accepted via MediaCodec#configure() before the encoder is started
   */
  public static final String KEY_ER_SLICE_SPACING_SIZE = "vendor.qti-ext-enc-slice.spacing";


  /**
   * A key to specify the maximum number of LTR frames (slots) the encoder can store at any given time.
   * <p>
   * The associated value is an integer
   *
   * <p>
   * This key is accepted via MediaCodec#configure() before the encoder is started
   */
  public static final String KEY_ER_LTR_MAX_FRAMES = "vendor.qti-ext-enc-ltr-count.num-ltr-frames";

  /**
   * A key to specify the index of the slot to store the LTR Frame.
   * <p>
   * The associated value is an integer
   *
   * <p>
   * If the specified slot has a previously stored LTR frame, it will be replaced with the current frame
   * This key is accepted via MediaCodec#SetParameters() before the encoder is started
   */
  public static final String KEY_ER_LTR_MARK_FRAME = "vendor.qti-ext-enc-ltr.mark-frame";

  /**
   * A key to specify the slot index of a previously  stored LTR frame to be used as reference.
   * <p>
   * The associated value is an integer
   *
   * <p>
   * This key is accepted via MediaCodec#setParameters() with every frame
   */
  public static final String KEY_ER_LTR_USE_FRAME = "vendor.qti-ext-enc-ltr.use-frame";

  /**
   * The encoder use this key to signal a response to mark or use operations.
   * <p>
   * The associated value is an integer
   *
   * <p>
   * This key is accepted via MediaCodec#setParameters() with every frame
   */
  public static final String KEY_ER_LTR_RESPONSE = "vendor.qti-ext-enc-info-ltr.ltr-use-mark";

  /**
   * Vendor Extension for ProSight EncodingMode.
   * <p>
   * The associated value is an integer
   * <p>
   */
  public static final String KEY_PROSIGHT_ENCODER_MODE = "vendor.qti-ext-encoding-mode.value";

  /**
   * Enums for the ProSight extension
   */
  public enum ProSightExtensionRange {
    PROSIGHT(1), DEFAULT(0);
    private final int value;

    ProSightExtensionRange(int value) {
      this.value = value;
    }

    public int getValue() {
      return value;
    }
  }
}

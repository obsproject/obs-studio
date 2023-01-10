//
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

//-------------------------------------------------------------------------------------------------
//  VideoEncoderHW_AV1 interface declaration
//-------------------------------------------------------------------------------------------------

#ifndef AMF_VideoEncoderAV1_h
#define AMF_VideoEncoderAV1_h
#pragma once

#include "Component.h"
#include "ColorSpace.h"
#include "PreAnalysis.h"

#define AMFVideoEncoder_AV1 L"AMFVideoEncoderHW_AV1"

enum AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE_NONE                    = 0,    // No encoding latency requirement. Encoder will balance encoding time and power consumption.
    AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE_POWER_SAVING_REAL_TIME  = 1,    // Try the best to finish encoding a frame within 1/framerate sec. This mode may cause more power consumption
    AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE_REAL_TIME               = 2,    // Try the best to finish encoding a frame within 1/(2 x framerate) sec. This mode will cause more power consumption than POWER_SAVING_REAL_TIME
    AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE_LOWEST_LATENCY          = 3     // Encoding as fast as possible. This mode causes highest power consumption.
};

enum AMF_VIDEO_ENCODER_AV1_USAGE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_USAGE_TRANSCODING     = 0,
    AMF_VIDEO_ENCODER_AV1_USAGE_LOW_LATENCY     = 1
};

enum AMF_VIDEO_ENCODER_AV1_PROFILE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_PROFILE_MAIN = 1
};

enum AMF_VIDEO_ENCODER_AV1_LEVEL_ENUM
{
    AMF_VIDEO_ENCODER_AV1_LEVEL_2_0 = 0,
    AMF_VIDEO_ENCODER_AV1_LEVEL_2_1 = 1,
    AMF_VIDEO_ENCODER_AV1_LEVEL_2_2 = 2,
    AMF_VIDEO_ENCODER_AV1_LEVEL_2_3 = 3,
    AMF_VIDEO_ENCODER_AV1_LEVEL_3_0 = 4,
    AMF_VIDEO_ENCODER_AV1_LEVEL_3_1 = 5,
    AMF_VIDEO_ENCODER_AV1_LEVEL_3_2 = 6,
    AMF_VIDEO_ENCODER_AV1_LEVEL_3_3 = 7,
    AMF_VIDEO_ENCODER_AV1_LEVEL_4_0 = 8,
    AMF_VIDEO_ENCODER_AV1_LEVEL_4_1 = 9,
    AMF_VIDEO_ENCODER_AV1_LEVEL_4_2 = 10,
    AMF_VIDEO_ENCODER_AV1_LEVEL_4_3 = 11,
    AMF_VIDEO_ENCODER_AV1_LEVEL_5_0 = 12,
    AMF_VIDEO_ENCODER_AV1_LEVEL_5_1 = 13,
    AMF_VIDEO_ENCODER_AV1_LEVEL_5_2 = 14,
    AMF_VIDEO_ENCODER_AV1_LEVEL_5_3 = 15,
    AMF_VIDEO_ENCODER_AV1_LEVEL_6_0 = 16,
    AMF_VIDEO_ENCODER_AV1_LEVEL_6_1 = 17,
    AMF_VIDEO_ENCODER_AV1_LEVEL_6_2 = 18,
    AMF_VIDEO_ENCODER_AV1_LEVEL_6_3 = 19,
    AMF_VIDEO_ENCODER_AV1_LEVEL_7_0 = 20,
    AMF_VIDEO_ENCODER_AV1_LEVEL_7_1 = 21,
    AMF_VIDEO_ENCODER_AV1_LEVEL_7_2 = 22,
    AMF_VIDEO_ENCODER_AV1_LEVEL_7_3 = 23
};

enum AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_ENUM
{
    AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_UNKNOWN                   = -1,
    AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_CONSTANT_QP               = 0,
    AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_LATENCY_CONSTRAINED_VBR   = 1,
    AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR      = 2,
    AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_CBR                       = 3,
    AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_QUALITY_VBR               = 4,
    AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_HIGH_QUALITY_VBR          = 5,
    AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_HIGH_QUALITY_CBR          = 6
};

enum AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE_64X16_ONLY               = 1,
    AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE_64X16_1080P_CODED_1082   = 2,
    AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE_NO_RESTRICTIONS          = 3
};

enum AMF_VIDEO_ENCODER_AV1_FORCE_FRAME_TYPE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_FORCE_FRAME_TYPE_NONE             = 0,
    AMF_VIDEO_ENCODER_AV1_FORCE_FRAME_TYPE_KEY              = 1,
    AMF_VIDEO_ENCODER_AV1_FORCE_FRAME_TYPE_INTRA_ONLY       = 2,
    AMF_VIDEO_ENCODER_AV1_FORCE_FRAME_TYPE_SWITCH           = 3,
    AMF_VIDEO_ENCODER_AV1_FORCE_FRAME_TYPE_SHOW_EXISTING    = 4
};

enum AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_KEY             = 0,
    AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_INTRA_ONLY      = 1,
    AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_INTER           = 2,
    AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_SWITCH          = 3,
    AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_SHOW_EXISTING   = 4
};

enum AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET_ENUM
{
    AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET_HIGH_QUALITY   = 0,
    AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET_QUALITY        = 30,
    AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET_BALANCED       = 70,
    AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET_SPEED          = 100
};

enum AMF_VIDEO_ENCODER_AV1_HEADER_INSERTION_MODE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_HEADER_INSERTION_MODE_NONE                = 0,
    AMF_VIDEO_ENCODER_AV1_HEADER_INSERTION_MODE_GOP_ALIGNED         = 1,
    AMF_VIDEO_ENCODER_AV1_HEADER_INSERTION_MODE_KEY_FRAME_ALIGNED   = 2
};

enum AMF_VIDEO_ENCODER_AV1_SWITCH_FRAME_INSERTION_MODE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_SWITCH_FRAME_INSERTION_MODE_NONE              = 0,
    AMF_VIDEO_ENCODER_AV1_SWITCH_FRAME_INSERTION_MODE_FIXED_INTERVAL    = 1
};

enum AMF_VIDEO_ENCODER_AV1_CDEF_MODE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_CDEF_DISABLE          = 0,
    AMF_VIDEO_ENCODER_AV1_CDEF_ENABLE_DEFAULT   = 1
};

enum AMF_VIDEO_ENCODER_AV1_CDF_FRAME_END_UPDATE_MODE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_CDF_FRAME_END_UPDATE_MODE_DISABLE         = 0,
    AMF_VIDEO_ENCODER_AV1_CDF_FRAME_END_UPDATE_MODE_ENABLE_DEFAULT  = 1
};

enum AMF_VIDEO_ENCODER_AV1_AQ_MODE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_AQ_MODE_NONE = 0,
    AMF_VIDEO_ENCODER_AV1_AQ_MODE_CAQ  = 1              // Content adaptive quantization mode
};

enum AMF_VIDEO_ENCODER_AV1_INTRA_REFRESH_MODE_ENUM
{
    AMF_VIDEO_ENCODER_AV1_INTRA_REFRESH_MODE__DISABLED = 0,
    AMF_VIDEO_ENCODER_AV1_INTRA_REFRESH_MODE__GOP_ALIGNED = 1,
    AMF_VIDEO_ENCODER_AV1_INTRA_REFRESH_MODE__CONTINUOUS = 2
};


// *** Static properties - can be set only before Init() ***

// Encoder Engine Settings
#define AMF_VIDEO_ENCODER_AV1_ENCODER_INSTANCE_INDEX                L"Av1EncoderInstanceIndex"          // amf_int64; default = 0; selected HW instance idx. The number of instances is queried by using AMF_VIDEO_ENCODER_AV1_CAP_NUM_OF_HW_INSTANCES
#define AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE                 L"Av1EncodingLatencyMode"           // amf_int64(AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE_ENUM); default = depends on USAGE; The encoding latency mode.
#define AMF_VIDEO_ENCODER_AV1_QUERY_TIMEOUT                         L"Av1QueryTimeout"                  // amf_int64; default = 0 (no wait); timeout for QueryOutput call in ms.

// Usage Settings
#define AMF_VIDEO_ENCODER_AV1_USAGE                                 L"Av1Usage"                         // amf_int64(AMF_VIDEO_ENCODER_AV1_USAGE_ENUM); default = N/A; Encoder usage. fully configures parameter set.

// Session Configuration
#define AMF_VIDEO_ENCODER_AV1_FRAMESIZE                             L"Av1FrameSize"                     // AMFSize; default = 0,0; Frame size
#define AMF_VIDEO_ENCODER_AV1_COLOR_BIT_DEPTH                       L"Av1ColorBitDepth"                 // amf_int64(AMF_COLOR_BIT_DEPTH_ENUM); default = AMF_COLOR_BIT_DEPTH_8
#define AMF_VIDEO_ENCODER_AV1_PROFILE                               L"Av1Profile"                       // amf_int64(AMF_VIDEO_ENCODER_AV1_PROFILE_ENUM) ; default = depends on USAGE; the codec profile of the coded bitstream
#define AMF_VIDEO_ENCODER_AV1_LEVEL                                 L"Av1Level"                         // amf_int64 (AMF_VIDEO_ENCODER_AV1_LEVEL_ENUM); default = depends on USAGE; the codec level of the coded bitstream
#define AMF_VIDEO_ENCODER_AV1_TILES_PER_FRAME                       L"Av1NumTilesPerFrame"              // amf_int64; default = 1; Number of tiles Per Frame. This is treated as suggestion. The actual number of tiles might be different due to compliance or encoder limitation.
#define AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET                        L"Av1QualityPreset"                 // amf_int64(AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET_ENUM); default = depends on USAGE; Quality Preset

// Codec Configuration
#define AMF_VIDEO_ENCODER_AV1_SCREEN_CONTENT_TOOLS                  L"Av1ScreenContentTools"            // bool; default = depends on USAGE; If true, allow enabling screen content tools by AMF_VIDEO_ENCODER_AV1_PALETTE_MODE_ENABLE and AMF_VIDEO_ENCODER_AV1_FORCE_INTEGER_MV; if false, all screen content tools are disabled.
#define AMF_VIDEO_ENCODER_AV1_ORDER_HINT                            L"Av1OrderHint"                     // bool; default = depends on USAGE; If true, code order hint; if false, don't code order hint
#define AMF_VIDEO_ENCODER_AV1_FRAME_ID                              L"Av1FrameId"                       // bool; default = depends on USAGE; If true, code frame id; if false, don't code frame id
#define AMF_VIDEO_ENCODER_AV1_TILE_GROUP_OBU                        L"Av1TileGroupObu"                  // bool; default = depends on USAGE; If true, code FrameHeaderObu + TileGroupObu and each TileGroupObu contains one tile; if false, code FrameObu.
#define AMF_VIDEO_ENCODER_AV1_CDEF_MODE                             L"Av1CdefMode"                      // amd_int64(AMF_VIDEO_ENCODER_AV1_CDEF_MODE_ENUM); default = depends on USAGE; Cdef mode
#define AMF_VIDEO_ENCODER_AV1_ERROR_RESILIENT_MODE                  L"Av1ErrorResilientMode"            // bool; default = depends on USAGE; If true, enable error resilient mode; if false, disable error resilient mode

// Rate Control and Quality Enhancement
#define AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD                   L"Av1RateControlMethod"             // amf_int64(AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_ENUM); default = depends on USAGE; Rate Control Method
#define AMF_VIDEO_ENCODER_AV1_QVBR_QUALITY_LEVEL                    L"Av1QvbrQualityLevel"              // amf_int64; default = 23; QVBR quality level; range = 1-51
#define AMF_VIDEO_ENCODER_AV1_INITIAL_VBV_BUFFER_FULLNESS           L"Av1InitialVBVBufferFullness"      // amf_int64; default = depends on USAGE; Initial VBV Buffer Fullness 0=0% 64=100%

// Alignment Mode Configuration
#define AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE                        L"Av1AlignmentMode"                 // amf_int64(AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE_ENUM); default = AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE_64X16_ONLY; Alignment Mode.

#define AMF_VIDEO_ENCODER_AV1_PRE_ANALYSIS_ENABLE                   L"Av1EnablePreAnalysis"             // bool; default = depends on USAGE; If true, enables the pre-analysis module. Refer to AMF Video PreAnalysis API reference for more details. If false, disable the pre-analysis module.
#define AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_PREENCODE                L"Av1RateControlPreEncode"          // bool; default = depends on USAGE; If true, enables pre-encode assist in rate control; if false, disables pre-encode assist in rate control.
#define AMF_VIDEO_ENCODER_AV1_HIGH_MOTION_QUALITY_BOOST             L"Av1HighMotionQualityBoost"        // bool; default = depends on USAGE; If true, enable high motion quality boost mode; if false, disable high motion quality boost mode.
#define AMF_VIDEO_ENCODER_AV1_AQ_MODE                               L"Av1AQMode"                        // amd_int64(AMF_VIDEO_ENCODER_AV1_AQ_MODE_ENUM); default = depends on USAGE; AQ mode

// Picture Management Configuration
#define AMF_VIDEO_ENCODER_AV1_MAX_NUM_TEMPORAL_LAYERS               L"Av1MaxNumOfTemporalLayers"        // amf_int64; default = depends on USAGE; Max number of temporal layers might be enabled. The maximum value can be queried from AMF_VIDEO_ENCODER_AV1_CAP_MAX_NUM_TEMPORAL_LAYERS
#define AMF_VIDEO_ENCODER_AV1_MAX_LTR_FRAMES                        L"Av1MaxNumLTRFrames"               // amf_int64; default = depends on USAGE; Max number of LTR frames. The maximum value can be queried from AMF_VIDEO_ENCODER_AV1_CAP_MAX_NUM_LTR_FRAMES
#define AMF_VIDEO_ENCODER_AV1_MAX_NUM_REFRAMES                      L"Av1MaxNumRefFrames"               // amf_int64; default = 1; Maximum number of reference frames

// color conversion
#define AMF_VIDEO_ENCODER_AV1_INPUT_HDR_METADATA                    L"Av1InHDRMetadata"                 // AMFBuffer containing AMFHDRMetadata; default NULL

// Miscellaneous
#define AMF_VIDEO_ENCODER_AV1_EXTRA_DATA                            L"Av1ExtraData"                     // AMFInterface* - > AMFBuffer*; buffer to retrieve coded sequence header


// *** Dynamic properties - can be set anytime ***

// Codec Configuration
#define AMF_VIDEO_ENCODER_AV1_PALETTE_MODE                          L"Av1PaletteMode"                   // bool; default = depends on USAGE; If true, enable palette mode; if false, disable palette mode. Valid only when AMF_VIDEO_ENCODER_AV1_SCREEN_CONTENT_TOOLS is true.
#define AMF_VIDEO_ENCODER_AV1_FORCE_INTEGER_MV                      L"Av1ForceIntegerMv"                // bool; default = depends on USAGE; If true, enable force integer MV; if false, disable force integer MV. Valid only when AMF_VIDEO_ENCODER_AV1_SCREEN_CONTENT_TOOLS is true.
#define AMF_VIDEO_ENCODER_AV1_CDF_UPDATE                            L"Av1CdfUpdate"                     // bool; default = depends on USAGE; If true, enable CDF update; if false, disable CDF update.
#define AMF_VIDEO_ENCODER_AV1_CDF_FRAME_END_UPDATE_MODE             L"Av1CdfFrameEndUpdateMode"         // amd_int64(AMF_VIDEO_ENCODER_AV1_CDF_FRAME_END_UPDATE_MODE_ENUM); default = depends on USAGE; CDF frame end update mode


// Rate Control and Quality Enhancement
#define AMF_VIDEO_ENCODER_AV1_VBV_BUFFER_SIZE                       L"Av1VBVBufferSize"                 // amf_int64; default = depends on USAGE; VBV Buffer Size in bits
#define AMF_VIDEO_ENCODER_AV1_FRAMERATE                             L"Av1FrameRate"                     // AMFRate; default = depends on usage; Frame Rate
#define AMF_VIDEO_ENCODER_AV1_ENFORCE_HRD                           L"Av1EnforceHRD"                    // bool; default = depends on USAGE; If true, enforce HRD; if false, HRD is not enforced.
#define AMF_VIDEO_ENCODER_AV1_FILLER_DATA                           L"Av1FillerData"                    // bool; default = depends on USAGE; If true, code filler data when needed; if false, don't code filler data.
#define AMF_VIDEO_ENCODER_AV1_TARGET_BITRATE                        L"Av1TargetBitrate"                 // amf_int64; default = depends on USAGE; Target bit rate in bits
#define AMF_VIDEO_ENCODER_AV1_PEAK_BITRATE                          L"Av1PeakBitrate"                   // amf_int64; default = depends on USAGE; Peak bit rate in bits

#define AMF_VIDEO_ENCODER_AV1_MAX_COMPRESSED_FRAME_SIZE             L"Av1MaxCompressedFrameSize"        // amf_int64; default = 0; Max compressed frame Size in bits. 0 - no limit
#define AMF_VIDEO_ENCODER_AV1_MIN_Q_INDEX_INTRA                     L"Av1MinQIndex_Intra"               // amf_int64; default = depends on USAGE; Min QIndex for intra frames; range = 0-255
#define AMF_VIDEO_ENCODER_AV1_MAX_Q_INDEX_INTRA                     L"Av1MaxQIndex_Intra"               // amf_int64; default = depends on USAGE; Max QIndex for intra frames; range = 0-255
#define AMF_VIDEO_ENCODER_AV1_MIN_Q_INDEX_INTER                     L"Av1MinQIndex_Inter"               // amf_int64; default = depends on USAGE; Min QIndex for inter frames; range = 0-255
#define AMF_VIDEO_ENCODER_AV1_MAX_Q_INDEX_INTER                     L"Av1MaxQIndex_Inter"               // amf_int64; default = depends on USAGE; Max QIndex for inter frames; range = 0-255

#define AMF_VIDEO_ENCODER_AV1_Q_INDEX_INTRA                         L"Av1QIndex_Intra"                  // amf_int64; default = depends on USAGE; intra-frame QIndex; range = 0-255
#define AMF_VIDEO_ENCODER_AV1_Q_INDEX_INTER                         L"Av1QIndex_Inter"                  // amf_int64; default = depends on USAGE; inter-frame QIndex; range = 0-255

#define AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_SKIP_FRAME               L"Av1RateControlSkipFrameEnable"    // bool; default = depends on USAGE; If true, rate control may code skip frame when needed; if false, rate control will not code skip frame.


// Picture Management Configuration
#define AMF_VIDEO_ENCODER_AV1_GOP_SIZE                              L"Av1GOPSize"                       // amf_int64; default = depends on USAGE; GOP Size (distance between automatically inserted key frames). If 0, key frame will be inserted at first frame only. Note that GOP may be interrupted by AMF_VIDEO_ENCODER_AV1_FORCE_FRAME_TYPE.
#define AMF_VIDEO_ENCODER_AV1_HEADER_INSERTION_MODE                 L"Av1HeaderInsertionMode"           // amf_int64(AMF_VIDEO_ENCODER_AV1_HEADER_INSERTION_MODE_ENUM); default = depends on USAGE; sequence header insertion mode
#define AMF_VIDEO_ENCODER_AV1_SWITCH_FRAME_INSERTION_MODE           L"Av1SwitchFrameInsertionMode"      // amf_int64(AMF_VIDEO_ENCODER_AV1_SWITCH_FRAME_INSERTION_MODE_ENUM); default = depends on USAGE; switch frame insertin mode
#define AMF_VIDEO_ENCODER_AV1_SWITCH_FRAME_INTERVAL                 L"Av1SwitchFrameInterval"           // amf_int64; default = depends on USAGE; the interval between two inserted switch frames. Valid only when AMF_VIDEO_ENCODER_AV1_SWITCH_FRAME_INSERTION_MODE is AMF_VIDEO_ENCODER_AV1_SWITCH_FRAME_INSERTION_MODE_FIXED_INTERVAL.
#define AMF_VIDEO_ENCODER_AV1_NUM_TEMPORAL_LAYERS                   L"Av1NumTemporalLayers"             // amf_int64; default = depends on USAGE; Number of temporal layers. Can be changed at any time but the change is only applied when encoding next base layer frame.

#define AMF_VIDEO_ENCODER_AV1_INTRA_REFRESH_MODE					L"Av1IntraRefreshMode"              // amf_int64(AMF_VIDEO_ENCODER_AV1_INTRA_REFRESH_MODE_ENUM); default AMF_VIDEO_ENCODER_AV1_INTRA_REFRESH_MODE__DISABLED
#define AMF_VIDEO_ENCODER_AV1_INTRAREFRESH_STRIPES					L"Av1IntraRefreshNumOfStripes"      // amf_int64; default = N/A; Valid only when intra refresh is enabled.

// color conversion
#define AMF_VIDEO_ENCODER_AV1_INPUT_COLOR_PROFILE                   L"Av1InputColorProfile"             // amf_int64(AMF_VIDEO_CONVERTER_COLOR_PROFILE_ENUM); default = AMF_VIDEO_CONVERTER_COLOR_PROFILE_UNKNOWN - mean AUTO by size
#define AMF_VIDEO_ENCODER_AV1_INPUT_TRANSFER_CHARACTERISTIC         L"Av1InputColorTransferChar"        // amf_int64(AMF_COLOR_TRANSFER_CHARACTERISTIC_ENUM); default = AMF_COLOR_TRANSFER_CHARACTERISTIC_UNDEFINED, ISO/IEC 23001-8_2013 section 7.2 See VideoDecoderUVD.h for enum
#define AMF_VIDEO_ENCODER_AV1_INPUT_COLOR_PRIMARIES                 L"Av1InputColorPrimaries"           // amf_int64(AMF_COLOR_PRIMARIES_ENUM); default = AMF_COLOR_PRIMARIES_UNDEFINED, ISO/IEC 23001-8_2013 section 7.1 See ColorSpace.h for enum

#define AMF_VIDEO_ENCODER_AV1_OUTPUT_COLOR_PROFILE                  L"Av1OutputColorProfile"            // amf_int64(AMF_VIDEO_CONVERTER_COLOR_PROFILE_ENUM); default = AMF_VIDEO_CONVERTER_COLOR_PROFILE_UNKNOWN - mean AUTO by size
#define AMF_VIDEO_ENCODER_AV1_OUTPUT_TRANSFER_CHARACTERISTIC        L"Av1OutputColorTransferChar"       // amf_int64(AMF_COLOR_TRANSFER_CHARACTERISTIC_ENUM); default = AMF_COLOR_TRANSFER_CHARACTERISTIC_UNDEFINED, ISO/IEC 23001-8_2013 ?7.2 See VideoDecoderUVD.h for enum
#define AMF_VIDEO_ENCODER_AV1_OUTPUT_COLOR_PRIMARIES                L"Av1OutputColorPrimaries"          // amf_int64(AMF_COLOR_PRIMARIES_ENUM); default = AMF_COLOR_PRIMARIES_UNDEFINED, ISO/IEC 23001-8_2013 section 7.1 See ColorSpace.h for enum


// Frame encode parameters
#define AMF_VIDEO_ENCODER_AV1_FORCE_FRAME_TYPE                      L"Av1ForceFrameType"                // amf_int64(AMF_VIDEO_ENCODER_AV1_FORCE_FRAME_TYPE_ENUM); default = AMF_VIDEO_ENCODER_AV1_FORCE_FRAME_TYPE_NONE; generate particular frame type
#define AMF_VIDEO_ENCODER_AV1_FORCE_INSERT_SEQUENCE_HEADER          L"Av1ForceInsertSequenceHeader"     // bool; default = false; If true, force insert sequence header with current frame;
#define AMF_VIDEO_ENCODER_AV1_MARK_CURRENT_WITH_LTR_INDEX           L"Av1MarkCurrentWithLTRIndex"       // amf_int64; default = N/A; Mark current frame with LTR index
#define AMF_VIDEO_ENCODER_AV1_FORCE_LTR_REFERENCE_BITFIELD          L"Av1ForceLTRReferenceBitfield"     // amf_int64; default = 0; force LTR bit-field
#define AMF_VIDEO_ENCODER_AV1_ROI_DATA                              L"Av1ROIData"					    // 2D AMFSurface, surface format: AMF_SURFACE_GRAY32

// Encode output parameters
#define AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE                     L"Av1OutputFrameType"               // amf_int64(AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_ENUM); default = N/A
#define AMF_VIDEO_ENCODER_AV1_OUTPUT_MARKED_LTR_INDEX               L"Av1MarkedLTRIndex"                // amf_int64; default = N/A; Marked LTR index
#define AMF_VIDEO_ENCODER_AV1_OUTPUT_REFERENCED_LTR_INDEX_BITFIELD  L"Av1ReferencedLTRIndexBitfield"    // amf_int64; default = N/A; referenced LTR bit-field

// AV1 Encoder capabilities - exposed in AMFCaps interface
#define AMF_VIDEO_ENCODER_AV1_CAP_NUM_OF_HW_INSTANCES               L"Av1CapNumOfHwInstances"           // amf_int64; default = N/A; number of HW encoder instances
#define AMF_VIDEO_ENCODER_AV1_CAP_MAX_THROUGHPUT                    L"Av1CapMaxThroughput"              // amf_int64; default = N/A; MAX throughput for AV1 encoder in MB (16 x 16 pixel)
#define AMF_VIDEO_ENCODER_AV1_CAP_REQUESTED_THROUGHPUT              L"Av1CapRequestedThroughput"        // amf_int64; default = N/A; Currently total requested throughput for AV1 encode in MB (16 x 16 pixel)
#define AMF_VIDEO_ENCODER_AV1_CAP_COLOR_CONVERSION                  L"Av1CapColorConversion"            // amf_int64(AMF_ACCELERATION_TYPE); default = N/A; type of supported color conversion. default AMF_ACCEL_GPU
#define AMF_VIDEO_ENCODER_AV1_CAP_PRE_ANALYSIS                      L"Av1PreAnalysis"                    // amf_bool - pre analysis module is available for AV1 UVE encoder, n/a for the other encoders
#define AMF_VIDEO_ENCODER_AV1_CAP_MAX_BITRATE                       L"Av1MaxBitrate"                    // amf_int64; default = N/A; Maximum bit rate in bits
#define AMF_VIDEO_ENCODER_AV1_CAP_MAX_PROFILE                       L"Av1MaxProfile"                    // amf_int64(AMF_VIDEO_ENCODER_AV1_PROFILE_ENUM); default = N/A; max value of code profile
#define AMF_VIDEO_ENCODER_AV1_CAP_MAX_LEVEL                         L"Av1MaxLevel"                      // amf_int64(AMF_VIDEO_ENCODER_AV1_LEVEL_ENUM); default = N/A; max value of codec level
#define AMF_VIDEO_ENCODER_AV1_CAP_MAX_NUM_TEMPORAL_LAYERS           L"Av1CapMaxNumTemporalLayers"       // amf_int64; default = N/A; The cap of maximum number of temporal layers
#define AMF_VIDEO_ENCODER_AV1_CAP_MAX_NUM_LTR_FRAMES                L"Av1CapMaxNumLTRFrames"            // amf_int64; default = N/A; The cap of maximum number of LTR frames. This value is calculated based on current value of AMF_VIDEO_ENCODER_AV1_MAX_NUM_TEMPORAL_LAYERS.

#endif //#ifndef AMF_VideoEncoderAV1_h

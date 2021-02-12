/*
 * A Plugin that integrates the AMD AMF encoder into OBS Studio
 * Copyright (C) 2016 - 2018 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#define P_TRANSLATE(x) obs_module_text(x)
#define P_DESC(x) x ".Description"

// Shared
#define P_VERSION "Version"
#define P_UTIL_DEFAULT "Utility.Default"
#define P_UTIL_AUTOMATIC "Utility.Automatic"
#define P_UTIL_MANUAL "Utility.Manual"
#define P_UTIL_SWITCH_DISABLED "Utility.Switch.Disabled"
#define P_UTIL_SWITCH_ENABLED "Utility.Switch.Enabled"

// Presets
#define P_PRESET "Preset"
#define P_PRESET_STREAMING "Preset.Streaming"
#define P_PRESET_RECORDING "Preset.Recording"
#define P_PRESET_LOWLATENCY "Preset.LowLatency"

// Static
#define P_QUALITYPRESET "QualityPreset"
#define P_QUALITYPRESET_SPEED "QualityPreset.Speed"
#define P_QUALITYPRESET_BALANCED "QualityPreset.Balanced"
#define P_QUALITYPRESET_QUALITY "QualityPreset.Quality"
#define P_PROFILE "Profile"
#define P_PROFILELEVEL "ProfileLevel"
#define P_TIER "Tier"
#define P_ASPECTRATIO "AspectRatio"
#define P_CODINGTYPE "CodingType"
#define P_CODINGTYPE_CABAC "CodingType.CABAC"
#define P_CODINGTYPE_CAVLC "CodingType.CAVLC"
#define P_MAXIMUMREFERENCEFRAMES "MaximumReferenceFrames"

// Rate Control
#define P_RATECONTROLMETHOD "RateControlMethod"
#define P_RATECONTROLMETHOD_CQP "RateControlMethod.CQP"
#define P_RATECONTROLMETHOD_CBR "RateControlMethod.CBR"
#define P_RATECONTROLMETHOD_VBR "RateControlMethod.VBR"
#define P_RATECONTROLMETHOD_VBRLAT "RateControlMethod.VBRLAT"
#define P_RATECONTROLMETHOD_QVBR "RateControlMethod.QVBR"
#define P_PREPASSMODE "PrePassMode"
#define P_BITRATE "bitrate"
#define P_BITRATE_TARGET "Bitrate.Target"
#define P_BITRATE_PEAK "Bitrate.Peak, %"
#define P_FILLERDATA "FillerData"
#define P_FRAMESKIPPING "FrameSkipping"
#define P_FRAMESKIPPING_PERIOD "FrameSkipping.Period"
#define P_FRAMESKIPPING_BEHAVIOUR "FrameSkipping.Behaviour"
#define P_FRAMESKIPPING_SKIPNTH "FrameSkipping.SkipNth"
#define P_FRAMESKIPPING_KEEPNTH "FrameSkipping.KeepNth"
#define P_VBAQ "VBAQ"
#define P_ENFORCEHRD "EnforceHRD"
#define P_HIGHMOTIONQUALITYBOOST "HighMotionQualityBoost"

// VBV Buffer
#define P_VBVBUFFER "VBVBuffer"
#define P_VBVBUFFER_SIZE "VBVBuffer.Size"

// Picture Control
#define P_INTERVAL_KEYFRAME "Interval.Keyframe"
#define P_PERIOD_IDR "Period.IDR"
#define P_INTERVAL_IFRAME "Interval.IFrame"
#define P_PERIOD_IFRAME "Period.IFrame"
#define P_INTERVAL_PFRAME "Interval.PFrame"
#define P_PERIOD_PFRAME "Period.PFrame"
#define P_INTERVAL_BFRAME "Interval.BFrame"
#define P_PERIOD_BFRAME "Period.BFrame"
#define P_GOP_TYPE "GOP.Type"                               // H265
#define P_GOP_TYPE_FIXED "GOP.Type.Fixed"                   // H265
#define P_GOP_TYPE_VARIABLE "GOP.Type.Variable"             // H265
#define P_GOP_SIZE "GOP.Size"                               // H265
#define P_GOP_SIZE_MINIMUM "GOP.Size.Minimum"               // H265
#define P_GOP_SIZE_MAXIMUM "GOP.Size.Maximum"               // H265
#define P_GOP_ALIGNMENT "GOP.Alignment"                     // Both?
#define P_BFRAME_PATTERN "BFrame.Pattern"                   // H264
#define P_BFRAME_DELTAQP "BFrame.DeltaQP"                   // H264
#define P_BFRAME_REFERENCE "BFrame.Reference"               // H264
#define P_BFRAME_REFERENCEDELTAQP "BFrame.ReferenceDeltaQP" // H264
#define P_DEBLOCKINGFILTER "DeblockingFilter"
#define P_MOTIONESTIMATION "MotionEstimation"
#define P_MOTIONESTIMATION_QUARTER "MotionEstimation.Quarter"
#define P_MOTIONESTIMATION_HALF "MotionEstimation.Half"
#define P_MOTIONESTIMATION_FULL "MotionEstimation.Full"

// System

#define P_VIEW "View"
#define P_VIEW_BASIC "View.Basic"
#define P_VIEW_ADVANCED "View.Advanced"
#define P_ENABLED "Enabled"
#define P_DISABLED "Disabled"
#define P_LOW_LATENCY_MODE "LowLatency"
#define P_LOG_LEVEL "LogLevel"
#define P_LOG_LEVEL_ERROR "LogLevel.ERROR"
#define P_LOG_LEVEL_WARNING "LogLevel.WARNING"
#define P_LOG_LEVEL_INFO "LogLevel.INFO"
#define P_LOG_LEVEL_DEBUG "LogLevel.DEBUG"
#define P_LOG_LEVEL_TRACE "LogLevel.TRACE"

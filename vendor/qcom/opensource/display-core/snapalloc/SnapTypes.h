// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_TYPES_H__
#define __SNAP_TYPES_H__

#include <Address.h>
#include <AllocationResult.h>
#include <BlendMode.h>
#include <BufferClient.h>
#include <BufferDescriptor.h>
#include <BufferLayout.h>
#include <BufferPermission.h>
#include <BufferUsage.h>
#include <CVPMetadata.h>
#include <ChromaSiting.h>
#include <Compression.h>
#include <CustomContentMetadata.h>
#include <Dataspace.h>
#include <Error.h>
#include <Fence.h>
#include <GraphicsMetadata.h>
#include <Interlaced.h>
#include <KeyValuePair.h>
#include <MetadataStatus.h>
#include <MetadataType.h>
#include <PixelFormat.h>
#include <PixelFormatModifier.h>
#include <PlaneLayout.h>
#include <PlaneLayoutComponent.h>
#include <PlaneLayoutComponentType.h>
#include <QtiColorPrimaries.h>
#include <QtiColorRange.h>
#include <QtiColorRemappingInfo.h>
#include <QtiContentLightLevel.h>
#include <QtiDynamicMetadata.h>
#include <QtiGammaTransfer.h>
#include <QtiMasteringDisplay.h>
#include <QtiMatrixCoEfficients.h>
#include <Rect.h>
#include <ReservedRegion.h>
#include <SnapHandle.h>
#include <UBWCStats.h>
#include <VideoHistogramMetadata.h>
#include <VideoTimestampInfo.h>
#include <VideoTranscodeStatsMetadata.h>
#include <QtiViews.h>
#include <ThreeDimensionalRefInfo.h>
#include <unordered_map>

using vendor::qti::hardware::display::snapalloc::AllocationResult;
using vendor::qti::hardware::display::snapalloc::BufferDescriptor;
using vendor::qti::hardware::display::snapalloc::Error;
using vendor::qti::hardware::display::snapalloc::SnapHandle;

namespace snapalloc {

enum {
  PRIV_FLAGS_CACHED = 0x00000200,
  PRIV_FLAGS_UBWC_ALIGNED = 0x08000000,
  PRIV_FLAGS_UBWC_ALIGNED_PI = 0x40000000,  // PI format
  PRIV_FLAGS_TILE_RENDERED = 0x02000000,
};

}  // namespace snapalloc

#endif  // __SNAP_TYPES_H__
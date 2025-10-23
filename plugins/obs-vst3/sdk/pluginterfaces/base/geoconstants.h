//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/geoconstants.h
// Created by  : Steinberg, 11/2014
// Description : Defines orientations and directions as also used by fpoint.h and frect.h
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------
namespace Steinberg {

//------------------------------------------------------------------------
enum Direction 
{
	kNorth,
	kNorthEast,
	kEast,
	kSouthEast,
	kSouth,
	kSouthWest,
	kWest,
	kNorthWest,
	kNoDirection,  //same position or center point of a geometry

	kNumberOfDirections,
};

//------------------------------------------------------------------------
enum Orientation 
{
	kHorizontal,
	kVertical,

	kNumberOfOrientations
};

//------------------------------------------------------------------------
namespace GeoConstants {

//------------------------------------------------------------------------
inline Direction toOpposite (Direction dir) 
{
	switch (dir) 
	{
		case kNorth :		return kSouth;
		case kNorthEast :	return kSouthWest;
		case kEast :		return kWest;
		case kSouthEast :	return kNorthWest;
		case kSouth :		return kNorth;
		case kSouthWest :	return kNorthEast;
		case kWest :		return kEast;
		case kNorthWest :	return kSouthEast;
		case kNoDirection : return kNoDirection;
		default:
			return kNumberOfDirections;
	}
}

//------------------------------------------------------------------------
inline Orientation toOrientation (Direction dir) 
{
	switch (dir) 
	{
		case kNorth :		return kVertical;
		case kEast :		return kHorizontal;
		case kSouth :		return kVertical;
		case kWest :		return kHorizontal;
		default:
			return kNumberOfOrientations;
	}
}

//------------------------------------------------------------------------
inline Orientation toOrthogonalOrientation (Orientation dir) 
{
	switch (dir) 
	{
		case kVertical :	return kHorizontal;
		case kHorizontal :	return kVertical;
		default:
			return kNumberOfOrientations;
	}
}

//------------------------------------------------------------------------
} // namespace GeoConstants
} // namespace Steinberg

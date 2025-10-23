//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/ucolorspec.h
// Created by  : Steinberg, 02/2006	(Host: S4.1)
// Description : GUI data types
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/ftypes.h"

namespace Steinberg {

//------------------------------------------------------------------------
// Colors		
typedef uint32			ColorSpec;
typedef uint8			ColorComponent;

typedef ColorSpec		UColorSpec;			// legacy support
typedef ColorComponent	UColorComponent;	// legacy support

/** Create color specifier with RGB values (alpha is opaque) */
inline SMTG_CONSTEXPR ColorSpec MakeColorSpec (ColorComponent r, ColorComponent g, ColorComponent b)
{ 
	return 0xFF000000 | ((ColorSpec)r) << 16 | ((ColorSpec)g) << 8 | (ColorSpec)b;
}
/** Create color specifier with RGBA values  */
inline SMTG_CONSTEXPR ColorSpec MakeColorSpec (ColorComponent r, ColorComponent g, ColorComponent b, ColorComponent a)
{ 
	return ((ColorSpec)a) << 24 | ((ColorSpec)r) << 16 | ((ColorSpec)g) << 8 | (uint32)b;
}

inline SMTG_CONSTEXPR ColorComponent GetBlue (ColorSpec cs)		         { return (ColorComponent)(cs & 0x000000FF); }
inline SMTG_CONSTEXPR ColorComponent GetGreen (ColorSpec cs)		     { return (ColorComponent)((cs >> 8) & 0x000000FF); }
inline SMTG_CONSTEXPR ColorComponent GetRed (ColorSpec cs)		         { return (ColorComponent)((cs >> 16) & 0x000000FF); }
inline SMTG_CONSTEXPR ColorComponent GetAlpha (ColorSpec cs)		     { return (ColorComponent)((cs >> 24) & 0x000000FF); }

inline void SetBlue (ColorSpec& argb, ColorComponent b)	 { argb = (argb & 0xFFFFFF00) | (ColorSpec)(b); }
inline void SetGreen (ColorSpec& argb, ColorComponent g)	 { argb = (argb & 0xFFFF00FF) | (((ColorSpec)g) << 8); }
inline void SetRed (ColorSpec& argb, ColorComponent r)	 { argb = (argb & 0xFF00FFFF) | (((ColorSpec)r) << 16); }
inline void SetAlpha (ColorSpec& argb, ColorComponent a)	 { argb = (argb & 0x00FFFFFF) | (((ColorSpec)a) << 24); }

/** Normalized color components*/
/** { */
inline double NormalizeColorComponent (ColorComponent c)    { return c / 255.0; }
inline ColorComponent DenormalizeColorComponent (double c)  { return static_cast<ColorComponent> (c * 255.0); }

inline void SetAlphaNorm (ColorSpec& argb, double a)        { SetAlpha (argb, DenormalizeColorComponent (a)); }
inline double GetAlphaNorm (ColorSpec cs)	                 { return (NormalizeColorComponent (GetAlpha (cs))); }
inline double NormalizeAlpha (uint8 alpha)                   {return NormalizeColorComponent (alpha);}
inline ColorComponent DenormalizeAlpha (double alphaNorm)   { return DenormalizeColorComponent (alphaNorm); }
/** } */
inline ColorSpec StripAlpha (ColorSpec argb)               { return (argb & 0x00FFFFFF); }

inline ColorSpec SMTG_CONSTEXPR BlendColor (ColorSpec color, double opacity)
{
	return MakeColorSpec (
		GetRed (color),
		GetGreen (color),
		GetBlue (color),
		static_cast<ColorComponent> (GetAlpha(color) * opacity)
	);
}

enum StandardColor	 //TODO_REFACTOR: change to enum class (c++11)
{
	kBlack = 0,
	kWhite,
	kGray5,
	kGray10,
	kGray20,
	kGray30,
	kGray40,
	kGray50,
	kGray60,
	kGray70,
	kGray80,
	kGray90,
	kRed,
	kLtRed,
	kDkRed,
	kGreen,
	kLtGreen,
	kDkGreen,
	kBlue,
	kLtBlue,
	kDkBlue,
	kMagenta,
	kLtMagenta,
	kDkMagenta,
	kYellow,
	kLtYellow,
	kDkYellow,
	kOrange,
	kLtOrange,
	kDkOrange,
	kGold,
	kBlack50,
	kBlack70,
	kNumStandardColors,
	kLtGray = kGray20,
	kGray = kGray50,
	kDkGray = kGray70
};

}

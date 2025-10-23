//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/keycodes.h
// Created by  : Steinberg, 01/2004
// Description : Key Code Definitions
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
//------------------------------------------------------------------------------
/** Virtual Key Codes.
OS-independent enumeration of virtual keycodes.
*/
enum VirtualKeyCodes
{
	KEY_BACK = 1,
	KEY_TAB,
	KEY_CLEAR,
	KEY_RETURN,
	KEY_PAUSE,
	KEY_ESCAPE,
	KEY_SPACE,
	KEY_NEXT,
	KEY_END,
	KEY_HOME,

	KEY_LEFT,
	KEY_UP,
	KEY_RIGHT,
	KEY_DOWN,
	KEY_PAGEUP,
	KEY_PAGEDOWN,

	KEY_SELECT,
	KEY_PRINT,
	KEY_ENTER,
	KEY_SNAPSHOT,
	KEY_INSERT,
	KEY_DELETE,
	KEY_HELP,
	KEY_NUMPAD0,
	KEY_NUMPAD1,
	KEY_NUMPAD2,
	KEY_NUMPAD3,
	KEY_NUMPAD4,
	KEY_NUMPAD5,
	KEY_NUMPAD6,
	KEY_NUMPAD7,
	KEY_NUMPAD8,
	KEY_NUMPAD9,
	KEY_MULTIPLY,
	KEY_ADD,
	KEY_SEPARATOR,
	KEY_SUBTRACT,
	KEY_DECIMAL,
	KEY_DIVIDE,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_NUMLOCK,
	KEY_SCROLL,

	KEY_SHIFT,
	KEY_CONTROL,
	KEY_ALT,

	KEY_EQUALS,				// only occurs on a Mac
	KEY_CONTEXTMENU,		// Windows only

	// multimedia keys
	KEY_MEDIA_PLAY,
	KEY_MEDIA_STOP,
	KEY_MEDIA_PREV,
	KEY_MEDIA_NEXT,
	KEY_VOLUME_UP,
	KEY_VOLUME_DOWN,

	KEY_F13,
	KEY_F14,
	KEY_F15,
	KEY_F16,
	KEY_F17,
	KEY_F18,
	KEY_F19,
	KEY_F20,
	KEY_F21,
	KEY_F22,
	KEY_F23,
	KEY_F24,

	KEY_SUPER,	// Win-Key on Windows, Ctrl-Key on macOS

	VKEY_FIRST_CODE = KEY_BACK,
	VKEY_LAST_CODE = KEY_SUPER,

	VKEY_FIRST_ASCII = 128
	/*
	 KEY_0 - KEY_9 are the same as ASCII '0' - '9' (0x30 - 0x39) + FIRST_ASCII
	 KEY_A - KEY_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A) + FIRST_ASCII
	*/
};

//------------------------------------------------------------------------------
inline SMTG_CONSTEXPR14 tchar VirtualKeyCodeToChar (uint8 vKey)
{
	if (vKey >= VKEY_FIRST_ASCII)
		return (tchar)(vKey - VKEY_FIRST_ASCII + 0x30);
	else if (vKey == KEY_SPACE)
		return ' ';
	return 0;
}

//------------------------------------------------------------------------------
inline SMTG_CONSTEXPR14 uint8 CharToVirtualKeyCode (tchar character)
{
	if ((character >= 0x30 && character <= 0x39) || (character >= 0x41 && character <= 0x5A))
		return (uint8)(character - 0x30 + VKEY_FIRST_ASCII);
	if (character == ' ')
		return (uint8)KEY_SPACE;
	return 0;
}

/** OS-independent enumeration of virtual modifier-codes. */
enum KeyModifier
{
	kShiftKey     = 1 << 0, ///< same on Windows and macOS
	kAlternateKey = 1 << 1, ///< same on Windows and macOS
	kCommandKey   = 1 << 2, ///< Windows: ctrl key; macOS: cmd key
	kControlKey   = 1 << 3  ///< Windows: win key, macOS: ctrl key
};

/** Simple data-struct representing a key-stroke on the keyboard. */
struct KeyCode
{
	/** The associated character. */
	tchar character {0};
	/** The associated virtual key-code. */
	uint8 virt {0};
	/** The associated virtual modifier-code. */
	uint8 modifier {0};

	/** Constructs a new KeyCode. */
	SMTG_CONSTEXPR14 KeyCode (tchar character = 0, uint8 virt = 0, uint8 modifier = 0)
	: character (character), virt (virt), modifier (modifier)
	{
	}

	SMTG_CONSTEXPR14 KeyCode& operator= (const KeyCode& other) SMTG_NOEXCEPT
	{
		character = other.character;
		virt = other.virt;
		modifier = other.modifier;

		return *this;
	}
};

/** Utility functions to handle key-codes. 
* @see Steinberg::KeyCode 
*/
namespace KeyCodes {}

namespace KeyCodes {

/** Is only a modifier pressed on the keyboard? */
template <typename Key>
bool isModifierOnlyKey (const Key& key)
{
	return (key.character == 0 && (key.virt == KEY_SHIFT || key.virt == KEY_ALT ||
	                               key.virt == KEY_CONTROL || key.virt == KEY_SUPER));
}

//------------------------------------------------------------------------
} // namespace KeyCodes
} // namespace Steinberg

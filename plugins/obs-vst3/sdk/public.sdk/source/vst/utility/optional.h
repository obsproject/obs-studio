//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/optional.h
// Created by  : Steinberg, 08/2016
// Description : optional helper
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <memory>
#include <utility>

//------------------------------------------------------------------------
namespace VST3 {

//------------------------------------------------------------------------
template <typename T>
struct Optional
{
	Optional () noexcept : valid (false) {}
	explicit Optional (const T& v) noexcept : _value (v), valid (true) {}
	Optional (T&& v) noexcept : _value (std::move (v)), valid (true) {}

	Optional (Optional&& other) noexcept { *this = std::move (other); }
	Optional& operator= (Optional&& other) noexcept
	{
		valid = other.valid;
		_value = std::move (other._value);
		return *this;
	}

	explicit operator bool () const noexcept
	{
		setValidationChecked ();
		return valid;
	}

	const T& operator* () const noexcept
	{
		checkValid ();
		return _value;
	}

	const T* operator-> () const noexcept
	{
		checkValid ();
		return &_value;
	}

	T& operator* () noexcept
	{
		checkValid ();
		return _value;
	}

	T* operator-> () noexcept
	{
		checkValid ();
		return &_value;
	}

	T&& value () noexcept
	{
		checkValid ();
		return std::move (_value);
	}

	const T& value () const noexcept
	{
		checkValid ();
		return _value;
	}

	void swap (T& other) noexcept
	{
		checkValid ();
		auto tmp = std::move (other);
		other = std::move (_value);
		_value = std::move (tmp);
	}

private:
	T _value {};
	bool valid;

#if !defined(NDEBUG)
	mutable bool validationChecked {false};
#endif

	void setValidationChecked () const
	{
#if !defined(NDEBUG)
		validationChecked = true;
#endif
	}
	void checkValid () const
	{
#if !defined(NDEBUG)
		assert (validationChecked);
#endif
	}
};

//------------------------------------------------------------------------
}

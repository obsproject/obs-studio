//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard

#pragma once

namespace Utility {
inline size_t HashRange(const uint32_t *const Begin, const uint32_t *const End, size_t Hash)
{
	// An inexpensive hash for CPUs lacking SSE4.2
	for (const uint32_t *Iter = Begin; Iter < End; ++Iter)
		Hash = 16777619U * Hash ^ *Iter;

	return Hash;
}

template<typename T> inline size_t HashState(const T *StateDesc, size_t Count = 1, size_t Hash = 2166136261U)
{
	static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
	return HashRange((uint32_t *)StateDesc, (uint32_t *)(StateDesc + Count), Hash);
}

} // namespace Utility

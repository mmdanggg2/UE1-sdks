/*=============================================================================
	UnMalloc.cpp: Unreal Tournament C++ Allocator Overrides
	Copyright 2024 OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Stijn Volckaert
=============================================================================*/

//
// This file globally overrides _all_ C++ (up to C++17) new/delete operators
// on non-Windows platforms.
//
// On Windows, we can just declare inline new/delete operators instead and
// include these inline definitions wherever we want them.
//
#if !_MSC_VER
#include "Core.h"
#include <new>
#include "FMallocImpl.h"
#define INLINE_OPERATORS 0
#include "MallocOverrides.h"
#endif
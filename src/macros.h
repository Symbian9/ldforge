/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2017 Teemu Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifndef __GNUC__
# define __attribute__(X)
#endif

#define DEFINE_CLASS(SELF, SUPER) \
public: \
	using Self = SELF; \
	using Super = SUPER;

#ifdef WIN32
# define DIRSLASH "\\"
# define DIRSLASH_CHAR '\\'
#else // WIN32
# define DIRSLASH "/"
# define DIRSLASH_CHAR '/'
#endif // WIN32

#define MAKE_ITERABLE_ENUM(T) \
	template<> \
	struct EnumLimits<T> \
	{\
	    enum { First = 0, Last = static_cast<int>(T::_End) - 1, Count = Last + 1 };\
	}; \
	inline T operator++ (T& a) { a = static_cast<T>(static_cast<int>(a) + 1); return a; } \
	inline T operator-- (T& a) { a = static_cast<T>(static_cast<int>(a) - 1); return a; } \
	inline T operator++ (T& a, int) { T result = a; a = static_cast<T>(static_cast<int>(a) + 1); return result; } \
	inline T operator-- (T& a, int) { T result = a; a = static_cast<T>(static_cast<int>(a) - 1); return result; }

template<typename T>
struct EnumLimits {};

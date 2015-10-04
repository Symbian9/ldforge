/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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
#include <type_traits>

#ifndef __GNUC__
# define __attribute__(X)
#endif

template <typename T, size_t N>
char (&countofHelper (T(&)[N]))[N];
#define countof(x) ((int) sizeof (countofHelper(x)))

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

#define dvalof(A) dprint ("value of '%1' = %2\n", #A, A)
#define for_axes(AX) for (const Axis AX : std::initializer_list<const Axis> ({X, Y, Z}))

#define MAKE_ITERABLE_ENUM(T) \
	inline T operator++ (T& a) { a = (T) ((int) a + 1); return a; } \
	inline T operator-- (T& a) { a = (T) ((int) a - 1); return a; } \
	inline T operator++ (T& a, int) { T result = a; a = (T) ((int) a + 1); return result; } \
	inline T operator-- (T& a, int) { T result = a; a = (T) ((int) a - 1); return result; }

#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
# define USE_QT5
#endif

#define FOR_ENUM_NAME_HELPER(LINE) enum_iterator_ ## LINE
#define FOR_ENUM_NAME(LINE) FOR_ENUM_NAME_HELPER(LINE)

#define for_enum(ENUM, NAME) \
	for (std::underlying_type<ENUM>::type FOR_ENUM_NAME (__LINE__) = \
		std::underlying_type<ENUM>::type (ENUM::FirstValue); \
		FOR_ENUM_NAME (__LINE__) < std::underlying_type<ENUM>::type (ENUM::NumValues); \
		++FOR_ENUM_NAME (__LINE__)) \
	for (ENUM NAME = ENUM (FOR_ENUM_NAME (__LINE__)); NAME != ENUM::NumValues; \
		NAME = ENUM::NumValues)

#define ConfigOption(...)

/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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

#define MAKE_ITERABLE_ENUM(T, FIRST, LAST) \
	template<> \
	struct EnumLimits<T> \
	{\
		enum { First = FIRST, Last = LAST, End = LAST + 1, Count = End - FIRST };\
	}; \
	inline T operator++ (T& a) { a = (T) ((int) a + 1); return a; } \
	inline T operator-- (T& a) { a = (T) ((int) a - 1); return a; } \
	inline T operator++ (T& a, int) { T result = a; a = (T) ((int) a + 1); return result; } \
	inline T operator-- (T& a, int) { T result = a; a = (T) ((int) a - 1); return result; }

template<typename T>
struct EnumLimits {};

#define ConfigOption(...)

#define DEFINE_FLAG_ACCESS_METHODS \
	bool checkFlag(Flag flag) const { return !!(m_flags & flag); } \
	void setFlag(Flag flag) { m_flags |= flag; } \
	void unsetFlag(Flag flag) { m_flags &= ~flag; }

// once-statement
struct OnceGuard
{
	bool triggered;
	OnceGuard() : triggered (false) {}

	bool pass()
	{
		if (triggered)
		{
			return false;
		}
		else
		{
			triggered = true;
			return true;
		}
	}
};

#define TEE_2(A,B) A ## B
#define TEE(A,B) TEE_2(A,B)
#define once static OnceGuard TEE(_once_, __LINE__); if (TEE(_once_, __LINE__).pass())

/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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

template<typename T>
struct EnumLimits {};

//
// Iterates an enum
//
template<typename Enum>
struct EnumIterShell
{
	struct Iterator
	{
		Iterator(typename std::underlying_type<Enum>::type i) :
			i(i) {}

		Iterator& operator++() { ++i; return *this; }
		bool operator==(Iterator other) { return i == other.i; }
		bool operator!=(Iterator other) { return i != other.i; }
		Enum operator*() const { return Enum(i); }

		typename std::underlying_type<Enum>::type i;
	};

	Iterator begin()
	{
		return Iterator(EnumLimits<Enum>::First);
	};

	Iterator end()
	{
		return Iterator(EnumLimits<Enum>::Last + 1);
	}
};

template<typename Enum>
EnumIterShell<Enum> iterateEnum()
{
	return EnumIterShell<Enum>();
}

/*
 * Casts an enum into its underlying type.
 */
template<typename T>
typename std::underlying_type<T>::type& enum_cast(T& enu)
{
	return *reinterpret_cast<typename std::underlying_type<T>::type*>(&enu);
}

/*
 * Returns whether an enum value is within proper bounds.
 */
template<typename Enum>
bool valueInEnum(Enum enumerator)
{
	auto index = enum_cast(enumerator);
	return index >= EnumLimits<Enum>::First and index <= EnumLimits<Enum>::Last;
}

#define MAKE_ITERABLE_ENUM(T) \
	template<> \
	struct EnumLimits<T> \
	{\
		enum { First = 0, Last = static_cast<int>(T::_End) - 1, Count = Last + 1 };\
	}; \
	inline T operator++ (T& a) { a = static_cast<T>(enum_cast(a) + 1); return a; } \
	inline T operator-- (T& a) { a = static_cast<T>(enum_cast(a) - 1); return a; } \
	inline T operator++ (T& a, int) { T result = a; a = static_cast<T>(enum_cast(a) + 1); return result; } \
	inline T operator-- (T& a, int) { T result = a; a = static_cast<T>(enum_cast(a) - 1); return result; }

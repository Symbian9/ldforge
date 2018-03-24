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

/*
 * reverse(container)
 *
 * Returns a container class that, when iterated, iterates over the container in reverse order.
 * Can be used for const and mutable containers, arrays as well as objects.
 */

template<typename T>
class ReverseIterator
{
public:
	ReverseIterator(T iterator) :
		iterator {iterator} {}

	ReverseIterator& operator++()
	{
		--this->iterator;
		return *this;
	}

	decltype(*T{}) operator*() const
	{
		return *this->iterator;
	}

	bool operator!=(const ReverseIterator<T>& other)
	{
		return this->iterator != other.iterator;
	}

private:
	T iterator;
};

template<typename T>
struct GenericIterator
{
	using type = decltype(std::begin(T{}));
};

template<typename T>
struct GenericConstIterator
{
	using type = decltype(std::begin((const T&) T{}));
};

template<typename T>
class ConstReverser
{
public:
	ConstReverser(const T& container) :
		container {container} {}

	using Iterator = ReverseIterator<typename GenericConstIterator<T>::type>;

	Iterator begin() const
	{
		auto result = std::end(this->container);
		return Iterator {--result};
	}

	Iterator end() const
	{
		auto result = std::begin(this->container);
		return Iterator {--result};
	}

private:
	const T& container;
};

template<typename T>
class MutableReverser
{
public:
	MutableReverser(T& container) :
		container {container} {}

	using Iterator = ReverseIterator<typename GenericIterator<T>::type>;

	Iterator begin() const
	{
		auto result = std::end(this->container);
		return Iterator {--result};
	}

	Iterator end() const
	{
		auto result = std::begin(this->container);
		return Iterator {--result};
	}

private:
	T& container;
};

template<typename T>
ConstReverser<T> reverse(const T& container)
{
	return {container};
}

template<typename T>
MutableReverser<T> reverse(T& container)
{
	return {container};
}

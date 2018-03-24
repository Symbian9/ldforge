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
#include <functional>

//
// TransformingIterator
//
// Transforming iterator, calls Fn on iterated values before returning them.
//
template<typename T, typename Res, typename Arg>
struct TransformingIterator
{
	using Iterator = decltype(T().begin());
	using Self = TransformingIterator<T, Res, Arg>;
	Iterator it;
	std::function<Res(Arg)> fn;

	TransformingIterator(Iterator it, std::function<Res(Arg)> fn) :
		it(it),
		fn(fn) {}

	Res operator*()
	{
		return fn(*it);
	}

	bool operator!= (const Self& other) const
	{
		return other.it != it;
	}

	void operator++()
	{
		++it;
	}
};

//
// TransformWrapper
//
// Transform object, only serves to produce transforming iterators
//
template<typename T, typename Res, typename Arg>
struct TransformWrapper
{
	using Iterator = TransformingIterator<T, Res, Arg>;

	TransformWrapper(T& iterable, std::function<Res(Arg)> fn) :
		iterable(iterable),
		function(fn) {}

	Iterator begin()
	{
		return Iterator(iterable.begin(), function);
	}

	Iterator end()
	{
		return Iterator(iterable.end(), function);
	}

	T& iterable;
	std::function<Res(Arg)> function;
};

template<typename T, typename Res, typename Arg>
TransformWrapper<T, Res, Arg> transform(T& iterable, Res(*fn)(Arg))
{
	return TransformWrapper<T, Res, Arg>(iterable, fn);
}

template<typename T, typename Res, typename Arg>
TransformWrapper<T, Res, Arg> transform(T& iterable, std::function<Res(Arg)> fn)
{
	return TransformWrapper<T, Res, Arg>(iterable, fn);
}

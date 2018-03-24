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
#include "../basics.h"

/*
 * Implements a ring adapter over T. This class corrects indices given to the element-operator so that they're within bounds.
 * The maximum amount can be specified manually.
 *
 * Example:
 *
 *   int A[] = {10,20,30,40};
 *   ring(A)[0]  ==     A[0 % 4]    == A[0]
 *   ring(A)[5]  ==     A[5 % 4]    == A[1]
 *   ring(A)[-1] == ring(A)[-1 + 4] == A[3]
 */
template<typename T>
class RingAdapter
{
private:
	// The private section must come first because _collection is used in decltype() below.
	T& _collection;
	const int _count;

public:
	RingAdapter(T& collection, int count) :
		_collection {collection},
		_count {count} {}

	template<typename IndexType>
	decltype(_collection[IndexType()]) operator[](IndexType index)
	{
		if (_count == 0)
		{
			// Argh! ...let the collection deal with this case.
			return _collection[0];
		}
		else
		{
			index %= _count;

			// Fix negative modulus...
			if (index < 0)
				index += _count;

			return _collection[index];
		}
	}

	int size() const
	{
		return _count;
	}
};

/*
 * Convenience function for RingAdapter so that the template parameter does not have to be provided. The ring amount is assumed
 * to be the amount of elements in the collection.
 */
template<typename T>
RingAdapter<T> ring(T& collection)
{
	return {collection, countof(collection)};
}

/*
 * Version of ring() that allows manual specification of the count.
 */
template<typename T>
RingAdapter<T> ring(T& collection, int count)
{
	return {collection, count};
}

template<typename T>
int countof(const RingAdapter<T>& ring)
{
	return ring.size();
}

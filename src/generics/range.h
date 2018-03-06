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

/*
 * Range
 *
 * This class models a range of values (by default integers but anything with a
 * total order qualifies) The value type must be constructible with 0 and 1 for
 * the sake of default values.
 *
 * The range may be iterated, in which case the first value yielded will be the
 * lower bound. Then, the iterator's value is incremented by a certain step
 * value, yielding the next value. This is continued until the iterator would
 * yield a value larger than upper bound.
 *
 * A range can be constructed in a number of ways:
 *	 - Range<>()
 *		 default-construction, lower and upper bounds will be set to 0.
 *	 - Range<>(first, end)
 *		 the range contains the values [first, end - 1].
 *		 Iteration of this range yields: first, first + 1, ..., end - 1.
 *	 - Range<>(first, second, end)
 *		 the range contains the values [first, end - 1].
 *		 Iteration of this range yields:
 *			 - first
 *			 - first + (second - first)
 *			 - first + 2*(second - first)
 *			 - end - 1 (assuming that (end - 1) can be represented with 
 *			 (first + n*(second - first)) with some n
 *
 * The function range() is provided to avoid the angle brackets for the common
 * use case where T = int.
 */
template<typename T = int>
class Range
{
	struct Iterator
	{
		const T baseValue;
		const T stepValue;
		int stepCount = 0;

		Iterator() :
			baseValue {0},
			stepValue {1},
			stepCount {0} {}

		Iterator(T value, T stepValue, T stepCount) :
			baseValue {value},
			stepValue {stepValue},
			stepCount {stepCount} {}

		T operator*() const
		{
			return baseValue + stepCount * stepValue;
		}

		bool operator!=(const Iterator& other) const
		{
			return stepCount != other.stepCount;
		}

		Iterator& operator++()
		{
			stepCount += 1;
			return *this;
		}

		Iterator& operator--()
		{
			stepCount -= 1;
			return *this;
		}
	};

public:
	Range() :
		beginValue {0},
		endValue {0},
		step {1} {}

	Range(T first, T second, T last) :
		beginValue {first},
		endValue {last + (second - first)},
		step {second - first} {}

	Iterator begin() const
	{
		return {beginValue, step, 0};
	}

	bool contains(T value) const
	{
		return value >= beginValue and value < endValue;
	}

	Iterator end() const
	{
		return {beginValue, step, (endValue - beginValue) / step};
	}

	bool overlaps(const Range<T>& other) const
	{
		return contains(other.beginValue) or contains(other.endValue - 1);
	}

	bool operator==(const Range<T>& other) const
	{
		return beginValue == other.beginValue
			and endValue == other.endValue
			and step == other.step;
	}

	bool operator!=(Range<T> const& other) const
	{
		return not operator==(other);
	}

private:
	T beginValue;
	T endValue;
	T step;
};

/*
 * Returns a range from [first, second, ..., last]
 */
template<typename T = int>
Range<T> range(T first, T second, T last)
{
	return {first, second, last};
}

/*
 * Returns a range from [first, first + 1, ..., end - 1]
 */
/*
template<typename T = int>
Range<T> range(T end)
{
	return range<T>(0, 1, end - 1);
}
*/

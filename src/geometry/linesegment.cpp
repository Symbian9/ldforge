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

#include "linesegment.h"

/*
 * Returns the vertices of this line segment as a QPair.
 */
QPair<Vertex, Vertex> LineSegment::toPair() const
{
	return {this->v_1, this->v_2};
}

/*
 * Possibly swaps the vertices of given line segment so that equivalent line segments become equal.
 */
LineSegment normalized(const LineSegment& segment)
{
	LineSegment result = segment;

	if (result.v_2 < result.v_1)
		qSwap(result.v_1, result.v_2);

	return result;
}

/*
 * Overload of qHash for line segments.
 */
unsigned int qHash(const LineSegment& segment)
{
	return qHash(normalized(segment).toPair());
}

/*
 * Comparison operator definition to allow line segments to be used in QSets.
 */
bool operator<(const LineSegment& one, const LineSegment& other)
{
	return normalized(one).toPair() < normalized(other).toPair();
}

/*
 * Checks whether two line segments are equal.
 */
bool operator==(const LineSegment& one, const LineSegment& other)
{
	return (one.v_1 == other.v_1 and one.v_2 == other.v_2)
		or (one.v_2 == other.v_1 and one.v_1 == other.v_2);
}

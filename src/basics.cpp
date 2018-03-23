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

#include "miscallenous.h"
#include "linetypes/modelobject.h"
#include "lddocument.h"

// http://stackoverflow.com/a/18204188/3629665
template<typename T>
inline int rotl10(T x)
{
	return (((x) << 10) | (((x) >> 22) & 0x000000ff));
}

template<typename T>
inline int rotl20(T x)
{
	return (((x) << 20) | (((x) >> 12) & 0x000000ff));
}

uint qHash(const Vertex& key)
{
	return qHash(key.x) ^ rotl10(qHash(key.y)) ^ rotl20(qHash(key.z));
}

/*
 * getRadialPoint
 *
 * Gets an ordinate of a point in circle.
 * If func == sin, then this gets the Y co-ordinate, if func == cos, then X co-ordinate.
 */
double getRadialPoint(int segment, int divisions, double(*func)(double))
{
	return (*func)((segment * 2 * pi) / divisions);
}

/*
 * makeCircle
 *
 * Creates a possibly partial circle rim.
 * Divisions is how many segments the circle makes if up if it's full.
 * Segments is now many segments are added.
 * Radius is the radius of the circle.
 *
 * If divisions == segments, this yields a full circle rim.
 * The rendered circle is returned as a vector of lines.
 */
QVector<QLineF> makeCircle(int segments, int divisions, double radius)
{
	QVector<QLineF> lines;

	for (int i = 0; i < segments; ++i)
	{
		double x0 = radius * getRadialPoint(i, divisions, cos);
		double x1 = radius * getRadialPoint(i + 1, divisions, cos);
		double z0 = radius * getRadialPoint(i, divisions, sin);
		double z1 = radius * getRadialPoint(i + 1, divisions, sin);
		lines.append(QLineF {QPointF {x0, z0}, QPointF {x1, z1}});
	}

	return lines;
}

/*
 * Computes the shortest distance from a point to a rectangle.
 *
 * The code originates from the Unity3D wiki, and was translated from C# to Qt by me (Teemu Piippo):
 *
 * Original code:
 *     http://wiki.unity3d.com/index.php/Distance_from_a_point_to_a_rectangle
 *
 * Copyright 2013 Philip Peterson.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
qreal distanceFromPointToRectangle(const QPointF& point, const QRectF& rectangle)
{
	//  Calculate a distance between a point and a rectangle.
	//  The area around/in the rectangle is defined in terms of
	//  several regions:
	//
	//  O--x
	//  |
	//  y
	//
	//
	//        I   |    II    |  III
	//      ======+==========+======   --yMin
	//       VIII |  IX (in) |  IV
	//      ======+==========+======   --yMax
	//       VII  |    VI    |   V
	//
	//
	//  Note that the +y direction is down because of Unity's GUI coordinates.

	if (point.x() < rectangle.left())
	{
		// Region I, VIII, or VII
		if (point.y() < rectangle.top()) // I
			return QLineF {point, rectangle.topLeft()}.length();
		else if (point.y() > rectangle.bottom()) // VII
			return QLineF {point, rectangle.bottomLeft()}.length();
		else // VIII
			return rectangle.left() - point.x();
	}
	else if (point.x() > rectangle.right())
	{
		// Region III, IV, or V
		if (point.y() < rectangle.top()) // III
			return QLineF {point, rectangle.topRight()}.length();
		else if (point.y() > rectangle.bottom()) // V
			return QLineF {point, rectangle.bottomRight()}.length();
		else // IV
			return point.x() - rectangle.right();
	}
	else
	{
		// Region II, IX, or VI
		if (point.y() < rectangle.top()) // II
			return rectangle.top() - point.y();
		else if (point.y() > rectangle.bottom()) // VI
			return point.y() - rectangle.bottom();
		else // IX
			return 0;
	}
}

/*
 * Special operator definition that implements the XOR operator for windings.
 * However, if either winding is NoWinding, then this function returns NoWinding.
 */
Winding operator^(Winding one, Winding other)
{
	if (one == NoWinding or other == NoWinding)
		return NoWinding;
	else
		return static_cast<Winding>(static_cast<int>(one) ^ static_cast<int>(other));
}

Winding& operator^=(Winding& one, Winding other)
{
	one = one ^ other;
	return one;
}

QDataStream& operator<<(QDataStream& out, const Library& library)
{
	return out << library.path << library.role;
}

QDataStream& operator>>(QDataStream &in, Library& library)
{
	return in >> library.path >> enum_cast<>(library.role);
}

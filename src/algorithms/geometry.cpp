#include "geometry.h"

/*
 * LDraw uses 4 points of precision for sin and cos values. Primitives must be generated
 * accordingly.
 */
double ldrawsin(double angle)
{
	return roundToDecimals(sin(angle), 4);
}

double ldrawcos(double angle)
{
	return roundToDecimals(cos(angle), 4);
}

/*
 * Returns a point on a circumference. LDraw precision is used.
 */
QPointF pointOnLDrawCircumference(int segment, int divisions)
{
	double angle = (segment * 2 * pi) / divisions;
	return {ldrawcos(angle), ldrawsin(angle)};
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

	for (int i = 0; i < segments; i += 1)
	{
		QPointF p0 = radius * ::pointOnLDrawCircumference(i, divisions);
		QPointF p1 = radius * ::pointOnLDrawCircumference(i + 1, divisions);
		lines.append({p0, p1});
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
	//  - I don't care which way is +y in this function because I want a distance --TP

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

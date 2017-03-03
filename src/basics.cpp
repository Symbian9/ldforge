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

Vertex::Vertex() :
	QVector3D() {}

Vertex::Vertex (const QVector3D& a) :
	QVector3D (a) {}

Vertex::Vertex (qreal xpos, qreal ypos, qreal zpos) :
	QVector3D(xpos, ypos, zpos) {}


void Vertex::transform (const Matrix& matrix, const Vertex& pos)
{
	double x2 = (matrix(0, 0) * x()) + (matrix(0, 1) * y()) + (matrix(0, 2) * z()) + pos.x();
	double y2 = (matrix(1, 0) * x()) + (matrix(1, 1) * y()) + (matrix(1, 2) * z()) + pos.y();
	double z2 = (matrix(2, 0) * x()) + (matrix(2, 1) * y()) + (matrix(2, 2) * z()) + pos.z();
	setX (x2);
	setY (y2);
	setZ (z2);
}

void Vertex::apply (ApplyFunction func)
{
	double newX = x(), newY = y(), newZ = z();
	func (X, newX);
	func (Y, newY);
	func (Z, newZ);
	*this = Vertex (newX, newY, newZ);
}

void Vertex::apply (ApplyConstFunction func) const
{
	func (X, x());
	func (Y, y());
	func (Z, z());
}

double Vertex::operator[] (Axis ax) const
{
	switch (ax)
	{
		case X: return x();
		case Y: return y();
		case Z: return z();
	}

	return 0.0;
}

void Vertex::setCoordinate (Axis ax, qreal value)
{
	switch (ax)
	{
		case X: setX (value); break;
		case Y: setY (value); break;
		case Z: setZ (value); break;
	}
}

QString Vertex::toString (bool mangled) const
{
	if (mangled)
		return format ("(%1, %2, %3)", x(), y(), z());

	return format ("%1 %2 %3", x(), y(), z());
}

Vertex Vertex::operator* (qreal scalar) const
{
	return Vertex (x() * scalar, y() * scalar, z() * scalar);
}

Vertex& Vertex::operator+= (const Vertex& other)
{
	setX (x() + other.x());
	setY (y() + other.y());
	setZ (z() + other.z());
	return *this;
}

Vertex Vertex::operator+ (const Vertex& other) const
{
	Vertex result (*this);
	result += other;
	return result;
}

Vertex& Vertex::operator*= (qreal scalar)
{
	setX (x() * scalar);
	setY (y() * scalar);
	setZ (z() * scalar);
	return *this;
}

bool Vertex::operator< (const Vertex& other) const
{
	if (x() != other.x()) return x() < other.x();
	if (y() != other.y()) return y() < other.y();
	if (z() != other.z()) return z() < other.z();
	return false;
}

/*
 * Transforms this vertex with a tranformation matrix and returns the result.
 */
Vertex Vertex::transformed(const GLRotationMatrix& matrix) const
{
	return {
		matrix(0, 0) * x() + matrix(0, 1) * y() + matrix(0, 2) * z(),
		matrix(1, 0) * x() + matrix(1, 1) * y() + matrix(1, 2) * z(),
		matrix(2, 0) * x() + matrix(2, 1) * y() + matrix(2, 2) * z(),
	};
}

// =============================================================================
//
BoundingBox::BoundingBox()
{
	reset();
}

// =============================================================================
//
BoundingBox& BoundingBox::operator<< (const Vertex& v)
{
	calcVertex (v);
	return *this;
}

// =============================================================================
//
void BoundingBox::calcVertex (const Vertex& vertex)
{
	m_vertex0.setX (qMin (vertex.x(), m_vertex0.x()));
	m_vertex0.setY (qMin (vertex.y(), m_vertex0.y()));
	m_vertex0.setZ (qMin (vertex.z(), m_vertex0.z()));
	m_vertex1.setX (qMax (vertex.x(), m_vertex1.x()));
	m_vertex1.setY (qMax (vertex.y(), m_vertex1.y()));
	m_vertex1.setZ (qMax (vertex.z(), m_vertex1.z()));
	m_isEmpty = false;
}

// =============================================================================
//
// Clears the bounding box
//
void BoundingBox::reset()
{
	m_vertex0 = Vertex (10000.0, 10000.0, 10000.0);
	m_vertex1 = Vertex (-10000.0, -10000.0, -10000.0);
	m_isEmpty = true;
}

// =============================================================================
//
// Returns the length of the bounding box on the longest measure.
//
double BoundingBox::longestMeasurement() const
{
	double xscale = (m_vertex0.x() - m_vertex1.x());
	double yscale = (m_vertex0.y() - m_vertex1.y());
	double zscale = (m_vertex0.z() - m_vertex1.z());
	double size = zscale;

	if (xscale > yscale)
	{
		if (xscale > zscale)
			size = xscale;
	}
	else if (yscale > zscale)
		size = yscale;

	if (qAbs (size) >= 2.0)
		return qAbs (size / 2);

	return 1.0;
}

// =============================================================================
//
// Yields the center of the bounding box.
//
Vertex BoundingBox::center() const
{
	return Vertex (
		(m_vertex0.x() + m_vertex1.x()) / 2,
		(m_vertex0.y() + m_vertex1.y()) / 2,
		(m_vertex0.z() + m_vertex1.z()) / 2);
}

bool BoundingBox::isEmpty() const
{
	return m_isEmpty;
}

const Vertex& BoundingBox::vertex0() const
{
	return m_vertex0;
}

const Vertex& BoundingBox::vertex1() const
{
	return m_vertex1;
}

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
	return qHash(key.x()) ^ rotl10(qHash(key.y())) ^ rotl20(qHash(key.z()));
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

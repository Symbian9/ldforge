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

#include "geometry.h"
#include "../linetypes/modelobject.h"
#include "../types/boundingbox.h"

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


void rotateVertex(Vertex& vertex, const Vertex& rotationPoint, const Matrix& transformationMatrix)
{
	vertex -= rotationPoint.toVector();
	vertex.transform (transformationMatrix, {0, 0, 0});
	vertex += rotationPoint.toVector();
}


void rotateObjects(int l, int m, int n, double angle, const QVector<LDObject*>& objects)
{
	Vertex rotationPoint = getRotationPoint (objects);
	double cosAngle = cos(angle);
	double sinAngle = sin(angle);

	// ref: http://en.wikipedia.org/wiki/Transformation_matrix#Rotation_2
	Matrix transformationMatrix (
	{
		(l * l * (1 - cosAngle)) + cosAngle,
		(m * l * (1 - cosAngle)) - (n * sinAngle),
		(n * l * (1 - cosAngle)) + (m * sinAngle),

		(l * m * (1 - cosAngle)) + (n * sinAngle),
		(m * m * (1 - cosAngle)) + cosAngle,
		(n * m * (1 - cosAngle)) - (l * sinAngle),

		(l * n * (1 - cosAngle)) - (m * sinAngle),
		(m * n * (1 - cosAngle)) + (l * sinAngle),
		(n * n * (1 - cosAngle)) + cosAngle
	});

	// Apply the above matrix to everything
	for (LDObject* obj : objects)
	{
		if (obj->numVertices())
		{
			for (int i = 0; i < obj->numVertices(); ++i)
			{
				Vertex v = obj->vertex (i);
				rotateVertex(v, rotationPoint, transformationMatrix);
				obj->setVertex (i, v);
			}
		}
		else if (obj->hasMatrix())
		{
			LDMatrixObject* mo = dynamic_cast<LDMatrixObject*> (obj);

			// Transform the position
			Vertex v = mo->position();
			rotateVertex(v, rotationPoint, transformationMatrix);
			mo->setPosition (v);

			// Transform the matrix
			mo->setTransformationMatrix(transformationMatrix * mo->transformationMatrix());
		}
	}
}


Vertex getRotationPoint(const QVector<LDObject*>& objs)
{
	switch (static_cast<RotationPoint>(config::rotationPointType()))
	{
	case ObjectOrigin:
		{
			BoundingBox box;

			// Calculate center vertex
			for (LDObject* obj : objs)
			{
				if (obj->hasMatrix())
				{
					box << static_cast<LDMatrixObject*> (obj)->position();
				}
				else
				{
					for (int i = 0; i < obj->numVertices(); ++i)
						box << obj->vertex(i);
				}
			}

			return box.center();
		}

	case WorldOrigin:
		return Vertex();

	case CustomPoint:
		return config::customRotationPoint();
	}

	return Vertex();
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

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

#include "plane.h"

/*
 * Constructs a plane from three points
 */
Plane Plane::fromPoints(const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
	Plane result;
	result.normal = QVector3D::crossProduct(v2 - v1, v3 - v1);
	result.point = v1;
	return result;
}

/*
 * Returns whether or not this plane is valid.
 */
bool Plane::isValid() const
{
	return not normal.isNull();
}

/*
 * Finds the intersection of a line and a plane. Returns a structure containing whether or not
 * there was an intersection as well as the intersected vertex if there was one. Can return a point
 * outside of the line segment.
 *
 * C.f. https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection#Algebraic_form
 */
Plane::IntersectionResult Plane::intersection(const Line& line) const
{
	if (isValid())
	{
		QVector3D lineVector = line.v_2 - line.v_1;
		qreal dot = QVector3D::dotProduct(lineVector, normal);

		if (not qFuzzyCompare(dot, 0.0))
		{
			qreal factor = QVector3D::dotProduct(point - line.v_1, normal) / dot;
			IntersectionResult result;
			result.intersected = true;
			result.vertex = Vertex::fromVector(lineVector * factor + (line.v_1 - Vertex {0, 0, 0}));
			return result;
		}
		else
		{
			// did not intersect
			return {};
		}
	}
	else
	{
		// plane is invalid
		return {};
	}
}

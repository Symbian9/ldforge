/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
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

#include "ldObjectMath.h"
#include "ldObject.h"
#include "miscallenous.h"

// =============================================================================
//
static void RotateVertex (Vertex& v, const Vertex& rotpoint, const Matrix& transformer)
{
	v -= rotpoint;
	v.transform (transformer, Origin);
	v += rotpoint;
}

// =============================================================================
//
void RotateObjects (const int l, const int m, const int n, double angle, LDObjectList const& objects)
{
	QList<Vertex*> queue;
	const Vertex rotpoint = GetRotationPoint (objects);
	const double cosangle = cos (angle),
				 sinangle = sin (angle);

	// ref: http://en.wikipedia.org/wiki/Transformation_matrix#Rotation_2
	Matrix transform (
	{
		(l * l * (1 - cosangle)) + cosangle,
		(m * l * (1 - cosangle)) - (n * sinangle),
		(n * l * (1 - cosangle)) + (m * sinangle),

		(l * m * (1 - cosangle)) + (n * sinangle),
		(m * m * (1 - cosangle)) + cosangle,
		(n * m * (1 - cosangle)) - (l * sinangle),

		(l * n * (1 - cosangle)) - (m * sinangle),
		(m * n * (1 - cosangle)) + (l * sinangle),
		(n * n * (1 - cosangle)) + cosangle
	});

	// Apply the above matrix to everything
	for (LDObject* obj : objects)
	{
		if (obj->numVertices())
		{
			for (int i = 0; i < obj->numVertices(); ++i)
			{
				Vertex v = obj->vertex (i);
				RotateVertex (v, rotpoint, transform);
				obj->setVertex (i, v);
			}
		}
		elif (obj->hasMatrix())
		{
			LDMatrixObject* mo = dynamic_cast<LDMatrixObject*> (obj);

			// Transform the position
			Vertex v = mo->position();
			RotateVertex (v, rotpoint, transform);
			mo->setPosition (v);

			// Transform the matrix
			mo->setTransform (transform * mo->transform());
		}
		elif (obj->type() == OBJ_Vertex)
		{
			LDVertex* vert = static_cast<LDVertex*> (obj);
			Vertex v = vert->pos;
			RotateVertex (v, rotpoint, transform);
			vert->pos = v;
		}
	}
}

/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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


MathFunctions::MathFunctions(QObject* parent) :
	HierarchyElement(parent) {}


void MathFunctions::rotateVertex(Vertex& vertex, const Vertex& rotationPoint, const Matrix& transformationMatrix) const
{
	vertex -= rotationPoint;
	vertex.transform (transformationMatrix, Origin);
	vertex += rotationPoint;
}


void MathFunctions::rotateObjects(int l, int m, int n, double angle, const LDObjectList& objects) const
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
			mo->setTransform(transformationMatrix * mo->transform());
		}
	}
}


Vertex MathFunctions::getRotationPoint(const LDObjectList& objs) const
{
	switch (RotationPoint (m_config->rotationPointType()))
	{
	case RotationPoint::ObjectOrigin:
		{
			BoundingBox box;

			// Calculate center vertex
			for (LDObject* obj : objs)
			{
				if (obj->hasMatrix())
					box << static_cast<LDMatrixObject*> (obj)->position();
				else
					box << obj;
			}

			return box.center();
		}

	case RotationPoint::WorldOrigin:
		return Vertex();

	case RotationPoint::CustomPoint:
		return m_config->customRotationPoint();

	case RotationPoint::NumValues:
		break;
	}

	return Vertex();
}
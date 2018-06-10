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

#pragma once
#include <QOpenGLFunctions>
#include <QGenericMatrix>
#include "basics.h"
#include "colors.h"
#include "generics/enums.h"
#include "types/vertex.h"

inline void glMultMatrixf(const GLRotationMatrix& matrix)
{
	glMultMatrixf(matrix.constData());
}

inline void glTranslatef(const Vertex& vertex)
{
	glTranslatef(vertex.x, vertex.y, vertex.z);
}

inline void glVertex(const Vertex& vertex)
{
	glVertex3f(vertex.x, vertex.y, vertex.z);
}

class LDObject;

struct LDPolygon
{
	enum class Type : qint8 { InvalidPolygon, EdgeLine, Triangle, Quadrilateral, ConditionalEdge };
	Type type = Type::InvalidPolygon;
	Vertex vertices[4];
	LDColor color;

	inline int numPolygonVertices() const
	{
		if (type == Type::ConditionalEdge)
			return 2;
		else
			return numVertices();
	}

	inline int numVertices() const
	{
		switch (type)
		{
		case Type::EdgeLine:
			return 2;

		case Type::Triangle:
			return 3;

		case Type::ConditionalEdge:
		case Type::Quadrilateral:
			return 4;

		case Type::InvalidPolygon:
			return 0;
		}

		return 0;
	}

	bool isValid() const
	{
		return type != Type::InvalidPolygon;
	}
};

Q_DECLARE_METATYPE(LDPolygon)

enum class VboClass
{
	Lines,
	Triangles,
	Quads,
	ConditionalLines,
	_End
};

MAKE_ITERABLE_ENUM(VboClass)

enum class VboSubclass
{
	Surfaces,
	RegularColors,
	PickColors,
	BfcFrontColors,
	BfcBackColors,
	RandomColors,
	Normals,
	InvertedNormals,
	_End
};

MAKE_ITERABLE_ENUM(VboSubclass)

enum
{
	NumVbos = EnumLimits<VboClass>::Count * EnumLimits<VboSubclass>::Count
};

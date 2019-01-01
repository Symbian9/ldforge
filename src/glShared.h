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

class LDObject;

namespace gl
{
	enum CameraType
	{
		TopCamera,
		FrontCamera,
		LeftCamera,
		BottomCamera,
		BackCamera,
		RightCamera,
		FreeCamera,
		_End
	};

	struct CameraIcon
	{
		QPixmap image;
		QRect sourceRect;
		QRect targetRect;
		QRect hitRect;
		CameraType camera;
	};

	class Renderer;
	class Compiler;

	static const QPen thinBorderPen {QColor {0, 0, 0, 208}, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin};

	// Transformation matrices for the fixed cameras.
	static const QMatrix4x4 topCameraMatrix = {};
	static const QMatrix4x4 frontCameraMatrix = {1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1};
	static const QMatrix4x4 leftCameraMatrix = {0, -1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0, 0, 0, 1};
	static const QMatrix4x4 bottomCameraMatrix = {1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1};
	static const QMatrix4x4 backCameraMatrix = {-1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1};
	static const QMatrix4x4 rightCameraMatrix = {0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1};

	// Conversion matrix from LDraw to OpenGL coordinates.
	static const QMatrix4x4 ldrawToGLAdapterMatrix = {1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1};

	enum { BlackRgb = 0xff000000 };
	static constexpr GLfloat near = 1.0f;
	static constexpr GLfloat far = 10000.0f;
}

MAKE_ITERABLE_ENUM(gl::CameraType)

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

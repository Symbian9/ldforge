/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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

#ifdef USE_QT5
# include <QOpenGLFunctions>
#endif

#include "basics.h"

class LDObject;

struct LDPolygon
{
	char		num;
	Vertex		vertices[4];
	int			id;
	int			color;

	inline int numVertices() const
	{
		return (num == 5) ? 4 : num;
	}
};

enum SurfaceVboType
{
	LinesVbo,
	TrianglesVbo,
	QuadsVbo,
	ConditionalLinesVbo,

	NumSurfaceVbos,
	FirstSurfaceVbo = LinesVbo
};

enum ComplementVboType
{
	SurfacesVboComplement,
	NormalColorsVboComplement,
	PickColorsVboComplement,
	BfcFrontColorsVboComplement,
	BfcBackColorsVboComplement,
	RandomColorsVboComplement,

	NumVboComplements,
	FirstVboComplement = SurfacesVboComplement
};

enum
{
	NumVbos = NumSurfaceVbos * NumVboComplements
};

MAKE_ITERABLE_ENUM (SurfaceVboType)
MAKE_ITERABLE_ENUM (ComplementVboType)

#ifndef USE_QT5
// Placeholder QOpenGLFunctions for Qt 4.x support
struct QOpenGLFunctions
{
	void initializeOpenGLFunctions() {}
};

#endif

// KDevelop doesn't seem to understand some VBO stuff
#ifdef IN_IDE_PARSER
using GLint = int;
using GLsizei = int;
using GLenum = unsigned int;
using GLuint = unsigned int;
void glBindBuffer (GLenum, GLuint);
void glGenBuffers (GLuint, GLuint*);
void glDeleteBuffers (GLuint, GLuint*);
void glBufferData (GLuint, GLuint, void*, GLuint);
void glBufferSubData (GLenum, GLint, GLsizei, void*);
#endif

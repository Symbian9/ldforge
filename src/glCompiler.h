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
#include "main.h"
#include "glRenderer.h"
#include "glShared.h"
#include <QMap>
#include <QSet>

// =============================================================================
//
class GLCompiler : public HierarchyElement, protected QOpenGLFunctions
{
public:
	struct ObjectVBOInfo
	{
		QVector<GLfloat>	data[NumVbos];
		bool				isChanged;
	};

	GLCompiler (GLRenderer* renderer);
	~GLCompiler();
	void compileDocument (LDDocument* doc);
	void dropObjectInfo (LDObject* obj);
	QColor getColorForPolygon (LDPolygon& poly, LDObject* topobj, ComplementVboType complement) const;
	QColor indexColorForID (int id) const;
	void initialize();
	void needMerge();
	void prepareVBO (int vbonum);
	void setRenderer (GLRenderer* compiler);
	void stageForCompilation (LDObject* obj);
	void unstage (LDObject* obj);
	GLuint vbo (int vbonum) const;
	int vboSize (int vbonum) const;

	static int vboNumber (SurfaceVboType surface, ComplementVboType complement);

private:
	void compileStaged();
	void compileObject (LDObject* obj);
	void compilePolygon (LDPolygon& poly, LDObject* topobj, ObjectVBOInfo* objinfo);

	QMap<LDObject*, ObjectVBOInfo>	m_objectInfo;
	QSet<LDObject*> m_staged; // Objects that need to be compiled
	GLuint m_vbo[NumVbos];
	bool m_vboChanged[NumVbos];
	int m_vboSizes[NumVbos];
	GLRenderer* m_renderer;
};

#define CHECK_GL_ERROR() { CheckGLErrorImpl (__FILE__, __LINE__); }
void CheckGLErrorImpl (const char* file, int line);

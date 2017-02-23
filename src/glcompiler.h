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

#pragma once
#include "main.h"
#include "glrenderer.h"
#include "glShared.h"
#include <QMap>
#include <QSet>

// =============================================================================
//
class GLCompiler : public QObject, public HierarchyElement, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	struct ObjectVBOInfo
	{
		QVector<GLfloat>	data[NumVbos];
		bool				isChanged;
	};

	GLCompiler (GLRenderer* renderer);
	~GLCompiler();
	QColor getColorForPolygon (LDPolygon& poly, LDObject* topobj, VboSubclass complement);
	QColor indexColorForID (int id) const;
	void initialize();
	void needMerge();
	void prepareVBO (int vbonum, const Model* model);
	void setRenderer (GLRenderer* compiler);
	void stageForCompilation (LDObject* obj);
	void unstage (LDObject* obj);
	GLuint vbo (int vbonum) const;
	int vboSize (int vbonum) const;

	static int vboNumber (VboClass surface, VboSubclass complement);

private:
	void compileStaged();
	void compilePolygon (LDPolygon& poly, LDObject* topobj, ObjectVBOInfo* objinfo);
	Q_SLOT void compileObject (LDObject* obj);
	Q_SLOT void recompile();
	void dropObjectInfo (LDObject* obj);
	Q_SLOT void forgetObject(LDObject* object);

private:
	QMap<LDObject*, ObjectVBOInfo>	m_objectInfo;
	QSet<LDObject*> m_staged; // Objects that need to be compiled
	GLuint m_vbo[NumVbos];
	bool m_vboChanged[NumVbos] = {true};
	int m_vboSizes[NumVbos] = {0};
	GLRenderer* m_renderer;
};

#define CHECK_GL_ERROR() { CheckGLErrorImpl (this, __FILE__, __LINE__); }
void CheckGLErrorImpl (HierarchyElement* element, const char* file, int line);

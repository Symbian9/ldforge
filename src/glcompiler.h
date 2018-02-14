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

/*
 * Compiles LDObjects into polygons for the GLRenderer to draw.
 */
class GLCompiler : public QObject, public HierarchyElement, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	GLCompiler (GLRenderer* renderer);
	~GLCompiler();

	void initialize();
	void prepareVBO (int vbonum);
	GLuint vbo (int vbonum) const;
	int vboSize (int vbonum) const;

	static int vboNumber (VboClass surface, VboSubclass complement);

private:
	struct ObjectVboData
	{
		QVector<GLfloat> data[NumVbos];
	};

	void compileStaged();
	void compilePolygon (LDPolygon& poly, LDObject* polygonOwner, ObjectVboData& objectInfo);
	Q_SLOT void compileObject (QModelIndex index);
	QColor getColorForPolygon (LDPolygon& poly, LDObject* topobj, VboSubclass complement);
	QColor indexColorForID (qint32 id) const;
	void needMerge();
	Q_SLOT void recompile();
	void dropObjectInfo (const QModelIndex &index);
	Q_SLOT void forgetObject(QModelIndex index);
	void stageForCompilation(QModelIndex index);
	void unstage (QModelIndex index);
	LDObject* resolveObject(const QModelIndex& index);

	QMap<QPersistentModelIndex, ObjectVboData> m_objectInfo;
	QSet<QPersistentModelIndex> m_staged; // Objects that need to be compiled
	GLuint m_vbo[NumVbos];
	bool m_vboChanged[NumVbos] = {true};
	int m_vboSizes[NumVbos] = {0};
	GLRenderer* m_renderer;
};

#define CHECK_GL_ERROR() { checkGLError(this, __FILE__, __LINE__); }
void checkGLError (HierarchyElement* element, QString file, int line);

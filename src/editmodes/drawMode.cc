/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QPainter>
#include <QMouseEvent>
#include "drawMode.h"
#include "../ldObject.h"
#include "../glRenderer.h"

DrawMode::DrawMode (GLRenderer* renderer) :
	Super (renderer) {}

EditModeType DrawMode::type() const
{
	return EditModeType::Draw;
}

void DrawMode::render (QPainter& painter) const
{
	QVector<Vertex> poly;
	QFontMetrics metrics = QFontMetrics (QFont());

	for (Vertex const& vert : m_drawedVerts)
		poly << vert;

	// Draw the cursor vertex as the last one in the list.
	if (poly.size() < 4)
		poly << renderer()->position3D();

	renderPolygon (painter, poly, true);
}

bool DrawMode::preAddVertex (Vertex const& pos)
{
	// If we picked an already-existing vertex, stop drawing
	for (Vertex& vert : m_drawedVerts)
	{
		if (vert == pos)
		{
			endDraw();
			return true;
		}
	}

	return false;
}

bool DrawMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	if (data.releasedButtons & Qt::LeftButton)
	{
		// If we have 4 verts, stop drawing.
		if (m_drawedVerts.size() >= 4)
		{
			endDraw();
			return true;
		}

		addDrawnVertex (renderer()->position3D());
		return true;
	}

	return false;
}

void DrawMode::endDraw()
{
	// Clean the selection and create the object
	QList<Vertex>& verts = m_drawedVerts;
	LDObjectList objs;

	switch (verts.size())
	{
		case 1:
		{
			// 1 vertex - add a vertex object
			LDVertexPtr obj = LDSpawn<LDVertex>();
			obj->pos = verts[0];
			obj->setColor (MainColor());
			objs << obj;
			break;
		}

		case 2:
		{
			// 2 verts - make a line
			LDLinePtr obj = LDSpawn<LDLine> (verts[0], verts[1]);
			obj->setColor (EdgeColor());
			objs << obj;
			break;
		}

		case 3:
		case 4:
		{
			LDObjectPtr obj = (verts.size() == 3) ?
				static_cast<LDObjectPtr> (LDSpawn<LDTriangle>()) :
				static_cast<LDObjectPtr> (LDSpawn<LDQuad>());

			obj->setColor (MainColor());

			for (int i = 0; i < verts.size(); ++i)
				obj->setVertex (i, verts[i]);

			objs << obj;
			break;
		}
	}

	finishDraw (objs);
}

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
#include "../miscallenous.h"

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
		poly << getCursorVertex();

	renderPolygon (painter, poly, true, true);
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

		addDrawnVertex (getCursorVertex());
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
			LDVertex* obj = LDSpawn<LDVertex>();
			obj->pos = verts[0];
			obj->setColor (MainColor());
			objs << obj;
			break;
		}

		case 2:
		{
			// 2 verts - make a line
			LDLine* obj = LDSpawn<LDLine> (verts[0], verts[1]);
			obj->setColor (EdgeColor());
			objs << obj;
			break;
		}

		case 3:
		case 4:
		{
			LDObject* obj = (verts.size() == 3) ?
				static_cast<LDObject*> (LDSpawn<LDTriangle>()) :
				static_cast<LDObject*> (LDSpawn<LDQuad>());

			obj->setColor (MainColor());

			for (int i = 0; i < verts.size(); ++i)
				obj->setVertex (i, verts[i]);

			objs << obj;
			break;
		}
	}

	finishDraw (objs);
}

template<typename _Type>
_Type IntervalClamp (_Type a, _Type interval)
{
	_Type remainder = a % interval;

	if (remainder >= float (interval / 2))
		a += interval;

	a -= remainder;
	return a;
}

Vertex DrawMode::getCursorVertex() const
{
	Vertex result = renderer()->position3D();

	if (renderer()->keyboardModifiers() & Qt::ControlModifier
		and not m_drawedVerts.isEmpty())
	{
		Vertex const& v0 = m_drawedVerts.last();
		Vertex const& v1 = result;
		Axis relX, relY;

		renderer()->getRelativeAxes (relX, relY);
		QLineF ln (v0[relX], v0[relY], v1[relX], v1[relY]);
		ln.setAngle (IntervalClamp<int> (ln.angle(), 45));
		result.setCoordinate (relX, Grid::Snap (ln.x2(), Grid::Coordinate));
		result.setCoordinate (relY, Grid::Snap (ln.y2(), Grid::Coordinate));
	}

	return result;
}

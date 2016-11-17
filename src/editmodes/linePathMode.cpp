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

#include <QKeyEvent>
#include "linePathMode.h"
#include "../glRenderer.h"
#include "../mainwindow.h"

LinePathMode::LinePathMode (GLRenderer *renderer) :
	Super (renderer) {}

void LinePathMode::render (QPainter& painter) const
{
	QVector<QPointF> points;
	QList<Vertex> points3d = m_drawedVerts;
	points3d << renderer()->position3D();

	for (Vertex const& vrt : points3d)
		points << renderer()->convert3dTo2d (vrt);

	painter.setPen (renderer()->textPen());

	if (points.size() == points3d.size())
	{
		for (int i = 0; i < points.size() - 1; ++i)
		{
			painter.drawLine (QLineF (points[i], points[i + 1]));
			drawLineLength (painter, points3d[i], points3d[i + 1], points[i], points[i + 1]);
		}
	
		for (int i = 0; i < points.size(); ++i)
		{
			const QPointF& point = points[i];
			renderer()->drawPoint (painter, point);
			renderer()->drawBlipCoordinates (painter, points3d[i], point);
		}
	}
}

bool LinePathMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	if (data.releasedButtons & Qt::LeftButton)
	{
		addDrawnVertex (renderer()->position3D());
		return true;
	}

	return false;
}

bool LinePathMode::preAddVertex (Vertex const& pos)
{
	// If we picked the vertex we last drew, stop drawing
	if (not m_drawedVerts.isEmpty() and pos == m_drawedVerts.last())
	{
		endDraw();
		return true;
	}

	return false;
}

void LinePathMode::endDraw()
{
	LDObjectList objs;

	for (int i = 0; i < m_drawedVerts.size() - 1; ++i)
	{
		LDLine* line = LDSpawn<LDLine>();
		line->setVertex (0, m_drawedVerts[i]);
		line->setVertex (1, m_drawedVerts[i + 1]);
		objs << line;
	}

	finishDraw (objs);
}

bool LinePathMode::keyReleased (QKeyEvent* ev)
{
	if (Super::keyReleased (ev))
		return true;

	if (ev->key() == Qt::Key_Enter or ev->key() == Qt::Key_Return)
	{
		endDraw();
		return true;
	}

	return false;
}

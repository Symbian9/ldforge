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

#include "curvemode.h"
#include "../ldObject.h"
#include "../glRenderer.h"

CurveMode::CurveMode (GLRenderer* renderer) :
	Super (renderer) {}

void CurveMode::render (QPainter& painter) const
{
	if (m_drawedVerts.size() >= 1)
	{
		Vertex curve[4];
		QPoint curve2d[4];

		for (int i = 0; i < qMin (countof(curve), m_drawedVerts.size()); ++i)
			curve[i] = m_drawedVerts[i];

		// Factor the cursor into the preview
		if (m_drawedVerts.size() < 4)
			curve[m_drawedVerts.size()] = getCursorVertex();

		// Default the control points to the first vertex position
		if (m_drawedVerts.size() < 2)
			curve[2] = curve[0];

		if (m_drawedVerts.size() < 3)
			curve[3] = curve[2];

		for (int i = 0; i < countof(curve); ++i)
			curve2d[i] = renderer()->convert3dTo2d (curve[i]);

		painter.setPen (QColor (0, 112, 112));
		if (m_drawedVerts.size() >= 2)
			painter.drawLine (curve2d[0], curve2d[2]);

		if (m_drawedVerts.size() >= 3)
			painter.drawLine (curve2d[1], curve2d[3]);

		for (int i = 0; i < qMin (countof(curve), m_drawedVerts.size() + 1); ++i)
		{
			if (i < 2)
				renderer()->drawBlip (painter, curve2d[i]);
			else
				// Give control points a different color
				renderer()->drawBlip (painter, curve2d[i], QColor (0, 112, 112));
			renderer()->drawBlipCoordinates (painter, curve[i], curve2d[i]);
		}

		QPainterPath path (curve2d[0]);
		path.cubicTo (curve2d[2], curve2d[3], curve2d[1]);
		painter.strokePath (path, renderer()->linePen());
	}
	else
	{
		// Even if we have nothing, still draw the vertex at the cursor
		QPoint vertex2d = renderer()->convert3dTo2d (getCursorVertex());
		renderer()->drawBlip (painter, vertex2d);
		renderer()->drawBlipCoordinates (painter, getCursorVertex(), vertex2d);
	}
}

EditModeType CurveMode::type() const
{
	return EditModeType::Curve;
}

void CurveMode::endDraw()
{
	if (m_drawedVerts.size() == 4)
	{
		LDObjectList objs;
		objs << LDSpawn<LDBezierCurve> (m_drawedVerts[0], m_drawedVerts[1], m_drawedVerts[2], m_drawedVerts[3]);
		finishDraw (objs);
	}
}

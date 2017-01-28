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

#include <QPainter>
#include <QMouseEvent>
#include "rectangleMode.h"
#include "../ldObject.h"
#include "../glRenderer.h"

RectangleMode::RectangleMode (GLRenderer* renderer) :
	Super (renderer),
	m_rectangleVerts (QVector<Vertex>(4)) {}

EditModeType RectangleMode::type() const
{
	return EditModeType::Rectangle;
}

void RectangleMode::render (QPainter& painter) const
{
	renderPolygon (painter, (length(m_drawedVerts) > 0) ? m_rectangleVerts :
		QVector<Vertex> ({renderer()->position3D()}), true, false);
}

void RectangleMode::endDraw()
{
	if (length(m_drawedVerts) == 2)
	{
		LDQuad* quad = LDSpawn<LDQuad>();
		updateRectVerts();

		for (int i = 0; i < quad->numVertices(); ++i)
			quad->setVertex (i, m_rectangleVerts[i]);

		quad->setColor (MainColor);
		finishDraw (LDObjectList ({quad}));
	}
}

//
// Update rect vertices when the mouse moves since the 3d position likely has changed
//
bool RectangleMode::mouseMoved (QMouseEvent*)
{
	updateRectVerts();
	return false;
}

void RectangleMode::updateRectVerts()
{
	if (m_drawedVerts.isEmpty())
	{
		for (int i = 0; i < 4; ++i)
			m_rectangleVerts[i] = renderer()->position3D();

		return;
	}

	Vertex v0 = m_drawedVerts[0],
		   v1 = (length(m_drawedVerts) >= 2) ? m_drawedVerts[1] : renderer()->position3D();

	const Axis localx = renderer()->getCameraAxis (false),
			   localy = renderer()->getCameraAxis (true),
			   localz = (Axis) (3 - localx - localy);

	for (int i = 0; i < 4; ++i)
		m_rectangleVerts[i].setCoordinate (localz, renderer()->getDepthValue());

	m_rectangleVerts[0].setCoordinate (localx, v0[localx]);
	m_rectangleVerts[0].setCoordinate (localy, v0[localy]);
	m_rectangleVerts[1].setCoordinate (localx, v1[localx]);
	m_rectangleVerts[1].setCoordinate (localy, v0[localy]);
	m_rectangleVerts[2].setCoordinate (localx, v1[localx]);
	m_rectangleVerts[2].setCoordinate (localy, v1[localy]);
	m_rectangleVerts[3].setCoordinate (localx, v0[localx]);
	m_rectangleVerts[3].setCoordinate (localy, v1[localy]);
}

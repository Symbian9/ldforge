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

#include <QMouseEvent>
#include <stdexcept>
#include "abstractEditMode.h"
#include "selectMode.h"
#include "drawMode.h"
#include "rectangleMode.h"
#include "circleMode.h"
#include "magicWandMode.h"
#include "lineLoopMode.h"
#include "../mainWindow.h"
#include "../glRenderer.h"

CFGENTRY (Bool, DrawLineLengths, true)
CFGENTRY (Bool, DrawAngles, false)

AbstractEditMode::AbstractEditMode (GLRenderer* renderer) :
	m_renderer (renderer) {}

AbstractEditMode::~AbstractEditMode() {}

AbstractEditMode* AbstractEditMode::createByType (GLRenderer* renderer, EditModeType type)
{
	switch (type)
	{
		case EditModeType::Select: return new SelectMode (renderer);
		case EditModeType::Draw: return new DrawMode (renderer);
		case EditModeType::Rectangle: return new RectangleMode (renderer);
		case EditModeType::Circle: return new CircleMode (renderer);
		case EditModeType::MagicWand: return new MagicWandMode (renderer);
		case EditModeType::LineLoop: return new LineLoopMode (renderer);
	}

	throw std::logic_error ("bad type given to AbstractEditMode::createByType");
}

GLRenderer* AbstractEditMode::renderer() const
{
	return m_renderer;
}

AbstractDrawMode::AbstractDrawMode (GLRenderer* renderer) :
	AbstractEditMode (renderer),
	m_polybrush (QBrush (QColor (64, 192, 0, 128)))
{
	// Disable the context menu - we need the right mouse button
	// for removing vertices.
	renderer->setContextMenuPolicy (Qt::NoContextMenu);

	// Use the crosshair cursor when drawing.
	renderer->setCursor (Qt::CrossCursor);

	// Clear the selection when beginning to draw.
	CurrentDocument()->clearSelection();

	g_win->updateSelection();
	m_drawedVerts.clear();
}

AbstractSelectMode::AbstractSelectMode (GLRenderer* renderer) :
	AbstractEditMode (renderer)
{
	renderer->unsetCursor();
	renderer->setContextMenuPolicy (Qt::DefaultContextMenu);
}

// =============================================================================
//
void AbstractDrawMode::addDrawnVertex (Vertex const& pos)
{
	if (preAddVertex (pos))
		return;

	m_drawedVerts << pos;
}

bool AbstractDrawMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	if ((data.releasedButtons & Qt::MidButton) and (m_drawedVerts.size() < 4) and (not data.mouseMoved))
	{
		// Find the closest vertex to our cursor
		double			minimumDistance = 1024.0;
		const Vertex*	closest = null;
		Vertex			cursorPosition = renderer()->coordconv2_3 (data.ev->pos(), false);
		QPoint			cursorPosition2D (data.ev->pos());
		const Axis		relZ = renderer()->getRelativeZ();
		QVector<Vertex>	vertices = renderer()->document()->inlineVertices();

		// Sort the vertices in order of distance to camera
		std::sort (vertices.begin(), vertices.end(), [&](const Vertex& a, const Vertex& b) -> bool
		{
			if (renderer()->getFixedCamera (renderer()->camera()).negatedDepth)
				return a[relZ] > b[relZ];

			return a[relZ] < b[relZ];
		});

		for (const Vertex& vrt : vertices)
		{
			// If the vertex in 2d space is very close to the cursor then we use
			// it regardless of depth.
			QPoint vect2d = renderer()->coordconv3_2 (vrt) - cursorPosition2D;
			const double distance2DSquared = std::pow (vect2d.x(), 2) + std::pow (vect2d.y(), 2);
			if (distance2DSquared < 16.0 * 16.0)
			{
				closest = &vrt;
				break;
			}

			// Check if too far away from the cursor.
			if (distance2DSquared > 64.0 * 64.0)
				continue;

			// Not very close to the cursor. Compare using true distance,
			// including depth.
			const double distanceSquared = (vrt - cursorPosition).lengthSquared();

			if (distanceSquared < minimumDistance)
			{
				minimumDistance = distanceSquared;
				closest = &vrt;
			}
		}

		if (closest != null)
			addDrawnVertex (*closest);

		return true;
	}

	if ((data.releasedButtons & Qt::RightButton) and (not m_drawedVerts.isEmpty()))
	{
		// Remove the last vertex
		m_drawedVerts.removeLast();

		return true;
	}

	return false;
}

void AbstractDrawMode::finishDraw (LDObjectList const& objs)
{
	if (objs.size() > 0)
	{
		for (LDObjectPtr obj : objs)
		{
			renderer()->document()->addObject (obj);
			renderer()->compileObject (obj);
		}

		g_win->refresh();
		g_win->endAction();
	}

	m_drawedVerts.clear();
}

void AbstractDrawMode::renderPolygon (QPainter& painter, const QVector<Vertex>& poly3d,
	bool withlengths, bool withangles) const
{
	QVector<QPoint> poly (poly3d.size());
	QFontMetrics metrics = QFontMetrics (QFont());

	// Convert to 2D
	for (int i = 0; i < poly3d.size(); ++i)
		poly[i] = renderer()->coordconv3_2 (poly3d[i]);

	// Draw the polygon-to-be
	painter.setBrush (m_polybrush);
	painter.drawPolygon (QPolygonF (poly));

	// Draw vertex blips
	for (int i = 0; i < poly3d.size(); ++i)
	{
		QPoint& blip = poly[i];
		painter.setPen (renderer()->linePen());
		renderer()->drawBlip (painter, blip);

		// Draw their coordinates
		painter.setPen (renderer()->textPen());
		painter.drawText (blip.x(), blip.y() - 8, poly3d[i].toString (true));
	}

	// Draw line lenghts and angle info if appropriate
	if (poly3d.size() >= 2 and (withlengths or withangles))
	{
		painter.setPen (renderer()->textPen());

		for (int i = 0; i < poly3d.size(); ++i)
		{
			const int j = (i + 1) % poly3d.size();
			const int h = (i - 1 >= 0) ? (i - 1) : (poly3d.size() - 1);

			if (withlengths and cfg::DrawLineLengths)
			{
				const QString label = QString::number ((poly3d[j] - poly3d[i]).length());
				QPoint origin = QLineF (poly[i], poly[j]).pointAt (0.5).toPoint();
				painter.drawText (origin, label);
			}

			if (withangles and cfg::DrawAngles)
			{
				QLineF l0 (poly[h], poly[i]),
					l1 (poly[i], poly[j]);

				double angle = 180 - l0.angleTo (l1);

				if (angle < 0)
					angle = 180 - l1.angleTo (l0);

				QString label = QString::number (angle) + QString::fromUtf8 (QByteArray ("\302\260"));
				QPoint pos = poly[i];
				pos.setY (pos.y() + metrics.height());

				painter.drawText (pos, label);
			}
		}
	}
}

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

#include <QMouseEvent>
#include <stdexcept>
#include "abstractEditMode.h"
#include "selectMode.h"
#include "drawMode.h"
#include "rectangleMode.h"
#include "circleMode.h"
#include "magicWandMode.h"
#include "linePathMode.h"
#include "curvemode.h"
#include "../mainwindow.h"
#include "../glRenderer.h"
#include "../miscallenous.h"
#include "../grid.h"

ConfigOption (bool DrawLineLengths = true)
ConfigOption (bool DrawAngles = false)

AbstractEditMode::AbstractEditMode(GLRenderer* renderer) :
	QObject(renderer),
	HierarchyElement(renderer),
	m_renderer(renderer) {}

AbstractEditMode::~AbstractEditMode() {}

AbstractEditMode* AbstractEditMode::createByType(GLRenderer* renderer, EditModeType type)
{
	switch (type)
	{
	case EditModeType::Select: return new SelectMode (renderer);
	case EditModeType::Draw: return new DrawMode (renderer);
	case EditModeType::Rectangle: return new RectangleMode (renderer);
	case EditModeType::Circle: return new CircleMode (renderer);
	case EditModeType::MagicWand: return new MagicWandMode (renderer);
	case EditModeType::LinePath: return new LinePathMode (renderer);
	case EditModeType::Curve: return new CurveMode (renderer);
	}

	throw std::logic_error("bad type given to AbstractEditMode::createByType");
}

GLRenderer* AbstractEditMode::renderer() const
{
	return m_renderer;
}

AbstractDrawMode::AbstractDrawMode(GLRenderer* renderer) :
	AbstractEditMode(renderer),
    m_polybrush{QBrush{QColor{64, 192, 0, 128}}}
{
	renderer->setContextMenuPolicy (Qt::NoContextMenu); // We need the right mouse button for removing vertices
	renderer->setCursor (Qt::CrossCursor);
	m_window->currentDocument()->clearSelection();
	m_window->updateSelection();
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
void AbstractDrawMode::addDrawnVertex(const Vertex& position)
{
	if (preAddVertex(position))
		return;

	m_drawedVerts << position;
}

bool AbstractDrawMode::mouseReleased(MouseEventData const& data)
{
	if (Super::mouseReleased(data))
		return true;

	if ((data.releasedButtons & Qt::MidButton) and (m_drawedVerts.size() < 4) and (not data.mouseMoved))
	{
		// Find the closest vertex to our cursor
		double minimumDistance = 1024.0;
		const Vertex* closest = nullptr;
		Vertex cursorPosition = renderer()->convert2dTo3d (data.ev->pos(), false);
		QPoint cursorPosition2D (data.ev->pos());
		const Axis depthAxis = renderer()->getRelativeZ();
		QList<Vertex> vertices = renderer()->document()->inlineVertices().toList();

		// Sort the vertices in order of distance to camera
		std::sort (vertices.begin(), vertices.end(), [&](const Vertex& a, const Vertex& b) -> bool
		{
			if (renderer()->cameraInfo (renderer()->camera()).negatedDepth)
				return a[depthAxis] > b[depthAxis];
			else
				return a[depthAxis] < b[depthAxis];
		});

		for (const Vertex& vrt : vertices)
		{
			// If the vertex in 2d space is very close to the cursor then we use it regardless of depth.
			QPoint vect2d = renderer()->convert3dTo2d (vrt) - cursorPosition2D;
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

		if (closest)
			addDrawnVertex (*closest);

		return true;
	}

	if ((data.releasedButtons & Qt::RightButton) and not m_drawedVerts.isEmpty())
	{
		// Remove the last vertex
		m_drawedVerts.removeLast();
		return true;
	}

	if (data.releasedButtons & Qt::LeftButton)
	{
		if (maxVertices() and m_drawedVerts.size() >= maxVertices())
			endDraw();
		else
			addDrawnVertex (getCursorVertex());
		return true;
	}

	return false;
}

void AbstractDrawMode::finishDraw(LDObjectList const& objs)
{
	int pos = m_window->suggestInsertPoint();

	if (objs.size() > 0)
	{
		for (LDObject* obj : objs)
		{
			renderer()->document()->insertObject (pos++, obj);
			renderer()->compileObject (obj);
		}

		m_window->refresh();
		m_window->endAction();
	}

	m_drawedVerts.clear();
}

void AbstractDrawMode::drawLineLength(QPainter &painter, const Vertex &v0, const Vertex &v1,
	const QPointF& v0p, const QPointF& v1p) const
{
	if (not m_config->drawLineLengths())
		return;

	const QString label = QString::number ((v1 - v0).length(), 'f', 2);
	QPoint origin = QLineF (v0p, v1p).pointAt (0.5).toPoint();
	painter.drawText (origin, label);
}

void AbstractDrawMode::renderPolygon(QPainter& painter, const QVector<Vertex>& polygon3d,
	bool drawLineLengths, bool drawAngles ) const
{
	QVector<QPoint> polygon2d (polygon3d.size());
	QFontMetrics metrics = QFontMetrics(QFont());

	// Convert to 2D
	for (int i = 0; i < polygon3d.size(); ++i)
		polygon2d[i] = renderer()->convert3dTo2d(polygon3d[i]);

	// Draw the polygon-to-be
	painter.setBrush(m_polybrush);
	painter.drawPolygon(QPolygonF{polygon2d});

	// Draw vertex blips
	for (int i = 0; i < polygon3d.size(); ++i)
	{
		renderer()->drawPoint(painter, polygon2d[i]);
		renderer()->drawBlipCoordinates(painter, polygon3d[i], polygon2d[i]);
	}

	// Draw line lenghts and angle info if appropriate
	if (polygon3d.size() >= 2 and (drawLineLengths or drawAngles))
	{
		painter.setPen (renderer()->textPen());

		for (int i = 0; i < polygon3d.size(); ++i)
		{
			int j = (i + 1) % polygon3d.size();
			int prior = (i - 1 >= 0) ? (i - 1) : (polygon3d.size() - 1);

			if (drawLineLengths)
				drawLineLength(painter, polygon3d[i], polygon3d[j], polygon2d[i], polygon2d[j]);

			if (drawAngles and m_config->drawAngles())
			{
				QLineF line0 = {polygon2d[prior], polygon2d[i]};
				QLineF line1 = {polygon2d[i], polygon2d[j]};
				double angle = 180 - line0.angleTo(line1);

				if (angle < 0)
					angle = 180 - line1.angleTo(line0);

				QString label = QString::number(angle) + "°";
				QPoint textPosition = polygon2d[i];
				textPosition.setY(textPosition.y() + metrics.height());
				painter.drawText(textPosition, label);
			}
		}
	}
}

bool AbstractDrawMode::keyReleased (QKeyEvent *ev)
{
	if (Super::keyReleased (ev))
		return true;

	if (not m_drawedVerts.isEmpty() and ev->key() == Qt::Key_Backspace)
	{
		m_drawedVerts.removeLast();
		return true;
	}

	return false;
}

//
// roundToInterval
//
// Rounds 'a' to the nearest multiple of 'interval'.
//
template<typename T>
T roundToInterval (T a, T interval)
{
	T remainder = a % interval;

	if (remainder >= interval / 2.0)
		a += interval;

	a -= remainder;
	return a;
}

Vertex AbstractDrawMode::getCursorVertex() const
{
	Vertex result = renderer()->position3D();

	if ((renderer()->keyboardModifiers() & Qt::ControlModifier) and not m_drawedVerts.isEmpty())
	{
		Vertex const& vertex0 = m_drawedVerts.last();
		Vertex const& vertex1 = result;
		Axis relativeX, relativeY;

		renderer()->getRelativeAxes (relativeX, relativeY);
		QLineF line = {vertex0[relativeX], vertex0[relativeY], vertex1[relativeX], vertex1[relativeY]};
		line.setAngle(roundToInterval<int>(line.angle(), 45));
		result.setCoordinate(relativeX, grid()->snap(line.x2(), Grid::Coordinate));
		result.setCoordinate(relativeY, grid()->snap(line.y2(), Grid::Coordinate));
	}

	return result;
}
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
#include "../lddocument.h"
#include "../canvas.h"
#include "../miscallenous.h"
#include "../grid.h"

/*
 * Base class constructor of the abstract editing mode.
 */
AbstractEditMode::AbstractEditMode(Canvas* canvas) :
    QObject(canvas),
    HierarchyElement(canvas),
    m_canvas(canvas) {}

/*
 * Constructs an edit mode by type.
 */
AbstractEditMode* AbstractEditMode::createByType(Canvas* canvas, EditModeType type)
{
	switch (type)
	{
	case EditModeType::Select: return new SelectMode (canvas);
	case EditModeType::Draw: return new DrawMode (canvas);
	case EditModeType::Rectangle: return new RectangleMode (canvas);
	case EditModeType::Circle: return new CircleMode (canvas);
	case EditModeType::MagicWand: return new MagicWandMode (canvas);
	case EditModeType::LinePath: return new LinePathMode (canvas);
	case EditModeType::Curve: return new CurveMode (canvas);
	}

	throw std::logic_error("bad type given to AbstractEditMode::createByType");
}

/*
 * Returns the edit mode's corresponding renderer pointer.
 */
Canvas* AbstractEditMode::renderer() const
{
	return m_canvas;
}

/*
 * Base class constructor of the abstract drwaing mode.
 */
AbstractDrawMode::AbstractDrawMode(Canvas* canvas) :
    AbstractEditMode {canvas},
    m_polybrush {QBrush {QColor {64, 192, 0, 128}}}
{
	canvas->setContextMenuPolicy(Qt::NoContextMenu); // We need the right mouse button for removing vertices
	canvas->setCursor(Qt::CrossCursor);
	m_window->currentDocument()->clearSelection();
	m_window->updateSelection();
	m_drawedVerts.clear();
}

/*
 * Base class constructor of the abstract selection mode.
 */
AbstractSelectMode::AbstractSelectMode(Canvas* canvas) :
    AbstractEditMode {canvas}
{
	canvas->unsetCursor();
	canvas->setContextMenuPolicy (Qt::DefaultContextMenu);
}

/*
 * Possibly adds this vertex into the list of drawn vertices.
 */
void AbstractDrawMode::addDrawnVertex(const Vertex& position)
{
	if (preAddVertex(position))
		return;

	m_drawedVerts << position;
}

/*
 * Handles mouse relese events.
 */
bool AbstractDrawMode::mouseReleased(MouseEventData const& data)
{
	if (Super::mouseReleased(data))
		return true;

	// If the user presses the middle mouse button, seek the closest existing vertex to the cursor and clamp to that.
	if ((data.releasedButtons & Qt::MidButton) and (countof(m_drawedVerts) < 4) and (not data.mouseMoved))
	{
		// Find the closest vertex to our cursor
		double minimumDistance = 1024.0;
		const Vertex* closest = nullptr;
		Vertex cursorPosition = renderer()->currentCamera().convert2dTo3d(data.ev->pos());
		QPoint cursorPosition2D = data.ev->pos();
		const Axis depthAxis = renderer()->getRelativeZ();
		QList<Vertex> vertices = currentDocument()->inlineVertices().toList();

		// Sort the vertices in order of distance to camera
		std::sort(vertices.begin(), vertices.end(), [&](const Vertex& a, const Vertex& b) -> bool
		{
			if (renderer()->currentCamera().isAxisNegated(Z))
				return a[depthAxis] > b[depthAxis];
			else
				return a[depthAxis] < b[depthAxis];
		});

		for (const Vertex& vertex : vertices)
		{
			// If the vertex in 2d space is very close to the cursor then we use it regardless of depth.
			QPoint vect2d = renderer()->currentCamera().convert3dTo2d(vertex) - cursorPosition2D;
			double distance2DSquared = std::pow(vect2d.x(), 2) + std::pow(vect2d.y(), 2);

			if (distance2DSquared < 16.0 * 16.0)
			{
				closest = &vertex;
				break;
			}

			// Check if too far away from the cursor.
			if (distance2DSquared > 64.0 * 64.0)
				continue;

			// Not very close to the cursor. Compare using true distance,
			// including depth.
			double distanceSquared = (vertex - cursorPosition).lengthSquared();

			if (distanceSquared < minimumDistance)
			{
				minimumDistance = distanceSquared;
				closest = &vertex;
			}
		}

		if (closest)
			addDrawnVertex(*closest);

		return true;
	}

	// If the user presses the right mouse button, remove the previously drawn vertex.
	if ((data.releasedButtons & Qt::RightButton) and not m_drawedVerts.isEmpty())
	{
		m_drawedVerts.removeLast();
		return true;
	}

	// If the user presses the left mouse button, insert the vertex or stop drawing, whichever is appropriate.
	if (data.releasedButtons & Qt::LeftButton)
	{
		if (maxVertices() and countof(m_drawedVerts) >= maxVertices())
			endDraw();
		else
			addDrawnVertex (getCursorVertex());

		return true;
	}

	// Otherwise we did not handle this mouse event.
	return false;
}

/*
 * Finalises the draw operation. The provided model is merged into the main document.
 */
void AbstractDrawMode::finishDraw(Model& model)
{
	int position = m_window->suggestInsertPoint();

	if (countof(model) > 0)
	{
		currentDocument()->merge(model, position);
		m_window->refresh();
		m_window->endAction();
	}

	m_drawedVerts.clear();
}

/*
 * Renders the length of the provided line.
 * - v0 and v1 are the line vertices in 3D space.
 * - v0p and v1p are the line vertices in 2D space (so that this function does not have to calculate them separately)
 */
void AbstractDrawMode::drawLineLength(QPainter &painter, const Vertex &v0, const Vertex &v1, const QPointF& v0p, const QPointF& v1p) const
{
	if (not m_config->drawLineLengths())
		return;

	const QString label = QString::number(abs(v1 - v0), 'f', 2);
	QPoint origin = QLineF {v0p, v1p}.pointAt(0.5).toPoint();
	painter.drawText (origin, label);
}

/*
 * Renders a polygon preview.
 *
 * painter: QPainter instance that is currently being rendered to.
 * polygon3d: The polygon as a vector of 3D vertices.
 * drawLineLengths: if true, lengths of polygon sides are also previewed, assuming the user has enabled the relevant option.
 * drawAngles: if true, the angles between polygon sides are also previewed, assuming the user has enabled the relevant option.
 */
void AbstractDrawMode::renderPolygon(QPainter& painter, const QVector<Vertex>& polygon3d, bool drawLineLengths, bool drawAngles) const
{
	QVector<QPoint> polygon2d {countof(polygon3d)};
	QFontMetrics metrics {QFont {}};

	// Convert to 2D
	for (int i = 0; i < countof(polygon3d); ++i)
		polygon2d[i] = renderer()->currentCamera().convert3dTo2d(polygon3d[i]);

	// Draw the polygon-to-be
	painter.setBrush(m_polybrush);
	painter.drawPolygon(QPolygonF{polygon2d});

	// Draw vertex blips
	for (int i = 0; i < countof(polygon3d); ++i)
	{
		renderer()->drawPoint(painter, polygon2d[i]);
		renderer()->drawBlipCoordinates(painter, polygon3d[i], polygon2d[i]);
	}

	// Draw line lenghts and angle info if appropriate
	if (countof(polygon3d) >= 2 and (drawLineLengths or drawAngles))
	{
		painter.setPen(renderer()->textPen());

		for (int i = 0; i < countof(polygon3d); ++i)
		{
			int j = (i + 1) % countof(polygon3d);
			int prior = (i - 1 >= 0) ? (i - 1) : (countof(polygon3d) - 1);

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

/*
 * Key release event handler
 */
bool AbstractDrawMode::keyReleased(QKeyEvent *event)
{
	if (Super::keyReleased(event))
		return true;

	// Map backspace to removing the previously drawn vertex.
	if (not m_drawedVerts.isEmpty() and event->key() == Qt::Key_Backspace)
	{
		m_drawedVerts.removeLast();
		return true;
	}

	return false;
}

/*
 * Computes the position for the vertex currently being drawn.
 */
Vertex AbstractDrawMode::getCursorVertex() const
{
	Vertex result = renderer()->position3D();

	// If the Ctrl key is pressed, then the vertex is locked to 45 degree angles relative to the previously drawn vertex.
	if ((renderer()->keyboardModifiers() & Qt::ControlModifier) and not m_drawedVerts.isEmpty())
	{
		const Vertex& vertex0 = m_drawedVerts.last();
		const Vertex& vertex1 = result;
		Axis relativeX;
		Axis relativeY;
		renderer()->getRelativeAxes(relativeX, relativeY);
		QLineF line = {vertex0[relativeX], vertex0[relativeY], vertex1[relativeX], vertex1[relativeY]};
		line.setAngle(roundToInterval<int>(line.angle(), 45));
		QPointF point = grid()->snap(line.p2());
		result.setCoordinate(relativeX, point.x());
		result.setCoordinate(relativeY, point.y());
	}

	return result;
}

/*
 * No draw mode can operate on the free camera, since 3D ⟷ 2D point conversions are not possible with it.
 */
bool AbstractDrawMode::allowFreeCamera() const
{
	return false;
}

/*
 * This virtual method allows drawing modes to specify how many vertices at most can be drawn. Returning 0 means unlimited.
 */
int AbstractDrawMode::maxVertices() const
{
	return 0;
}

/*
 * This virtual method is a hook that allows drawing modes to veto vertex insertion. Returning true means that the vertex insertion
 * was handled separately and the vertex will not be added.
 */
bool AbstractDrawMode::preAddVertex (Vertex const&)
{
	return false;
}

/*
 * This virtual method is overridden by subclasses to implement the actions taken by the edit mode.
 * The editing mode is to call finishDraw with the prepared model.
 *
 * TODO: the two method names are too similar and should be renamed.
 */
void AbstractDrawMode::endDraw() {}

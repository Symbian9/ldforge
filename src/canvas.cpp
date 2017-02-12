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
#include "canvas.h"
#include "documentmanager.h"
#include "grid.h"
#include "ldDocument.h"
#include "mainwindow.h"
#include "messageLog.h"
#include "miscallenous.h"
#include "primitives.h"

Canvas::Canvas(LDDocument* document, QWidget* parent) :
    GLRenderer {document, parent},
    m_document {*document},
    m_currentEditMode {AbstractEditMode::createByType (this, EditModeType::Select)} {}

Canvas::~Canvas()
{
	delete m_currentEditMode;
}

void Canvas::overpaint(QPainter& painter)
{
	GLRenderer::overpaint(painter);
	QFontMetrics metrics {QFont {}};

#ifndef RELEASE
	{
		QString text = format("Rotation: %1\nPanning: (%2, %3), Zoom: %4", rotationMatrix(), panning(X), panning(Y), zoom());
		QRect textSize = metrics.boundingRect(0, 0, width(), height(), Qt::AlignCenter, text);
		painter.setPen(textPen());
		painter.drawText((width() - textSize.width()) / 2, height() - textSize.height(), textSize.width(),
		    textSize.height(), Qt::AlignCenter, text);
	}
#endif

	if (camera() != Camera::Free)
	{
		// Paint the coordinates onto the screen.
		QString text = format(tr("X: %1, Y: %2, Z: %3"), m_position3D[X], m_position3D[Y], m_position3D[Z]);
		QFontMetrics metrics {font()};
		QRect textSize = metrics.boundingRect (0, 0, width(), height(), Qt::AlignCenter, text);
		painter.setPen(textPen());
		painter.drawText(width() - textSize.width(), height() - 16, textSize.width(), textSize.height(), Qt::AlignCenter, text);
	}

	// Draw edit mode HUD
	m_currentEditMode->render(painter);

	// Render triangle count
	{
		QPoint renderPoint {4, height() - 4 - metrics.height() - metrics.descent()};
		painter.drawText(renderPoint, format("△ %1", m_document.triangleCount()));
	}

	// Message log
	if (m_window->messageLog())
	{
		int y = 0;
		int margin = 2;
		QColor penColor = textPen().color();

		for (const MessageManager::Line& line : m_window->messageLog()->getLines())
		{
			penColor.setAlphaF(line.alpha);
			painter.setPen(penColor);
			painter.drawText(QPoint {margin, y + margin + metrics.ascent()}, line.text);
			y += metrics.height();
		}
	}
}

bool Canvas::freeCameraAllowed() const
{
	return m_currentEditMode->allowFreeCamera();
}

void Canvas::setEditMode(EditModeType a)
{
	if (m_currentEditMode and m_currentEditMode->type() == a)
		return;

	delete m_currentEditMode;
	m_currentEditMode = AbstractEditMode::createByType(this, a);

	// If we cannot use the free camera, use the top one instead.
	if (camera() == Camera::Free and not m_currentEditMode->allowFreeCamera())
		setCamera(Camera::Top);

	m_window->updateEditModeActions();
	update();
}

EditModeType Canvas::currentEditModeType() const
{
	return m_currentEditMode->type();
}

LDDocument* Canvas::document() const
{
	return &m_document;
}

void Canvas::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (m_currentEditMode->mouseDoubleClicked (event))
		event->accept();
}

void Canvas::dropEvent(QDropEvent* event)
{
	if (m_window and event->source() == m_window->getPrimitivesTree())
	{
		PrimitiveTreeItem* item = static_cast<PrimitiveTreeItem*> (m_window->getPrimitivesTree()->currentItem());
		QString primitiveName = item->primitive()->name;
		LDSubfileReference* reference = currentDocument()->emplaceAt<LDSubfileReference>(m_window->suggestInsertPoint());
		reference->setFileInfo (m_documents->getDocumentByName(primitiveName));
		currentDocument()->addToSelection(reference);
		m_window->buildObjectList();
		refresh();
		event->acceptProposedAction();
	}
}

void Canvas::keyReleaseEvent(QKeyEvent* event)
{
	m_currentEditMode->keyReleased(event);
	GLRenderer::keyReleaseEvent(event);
}

void Canvas::mouseMoveEvent(QMouseEvent* event)
{
	// Calculate 3d position of the cursor
	m_position3D = convert2dTo3d(mousePosition(), true);

	if (not m_currentEditMode->mouseMoved(event))
		GLRenderer::mouseMoveEvent(event);
}

void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
	AbstractEditMode::MouseEventData data;
	data.ev = event;
	data.mouseMoved = mouseHasMoved();
	data.keymods = keyboardModifiers();
	data.releasedButtons = lastButtons() & ~event->buttons();
	m_currentEditMode->mouseReleased(data);
	GLRenderer::mouseReleaseEvent(event);
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
	if (m_currentEditMode->mousePressed(event))
		event->accept();

	GLRenderer::mousePressEvent(event);
}

const Vertex& Canvas::position3D() const
{
	return m_position3D;
}

void Canvas::drawPoint(QPainter& painter, QPointF pos, QColor color) const
{
	int pointSize = 8;
	QPen pen = thinBorderPen;
	pen.setWidth(1);
	painter.setPen(pen);
	painter.setBrush(color);
	painter.drawEllipse(pos.x() - pointSize / 2, pos.y() - pointSize / 2, pointSize, pointSize);
}

void Canvas::drawBlipCoordinates(QPainter& painter, const Vertex& pos3d) const
{
	drawBlipCoordinates (painter, pos3d, convert3dTo2d (pos3d));
}

void Canvas::drawBlipCoordinates(QPainter& painter, const Vertex& pos3d, QPointF pos) const
{
	painter.setPen (textPen());
	painter.drawText (pos.x(), pos.y() - 8, pos3d.toString (true));
}

QPen Canvas::linePen() const
{
	QPen linepen = thinBorderPen;
	linepen.setWidth(2);
	linepen.setColor(luma(backgroundColor()) < 40 ? Qt::white : Qt::black);
	return linepen;
}


int Canvas::depthNegateFactor() const
{
	return cameraInfo(camera()).negatedDepth ? -1 : 1;
}

// =============================================================================
//
void Canvas::getRelativeAxes(Axis& relativeX, Axis& relativeY) const
{
	const CameraInfo& camera = cameraInfo(this->camera());
	relativeX = camera.localX;
	relativeY = camera.localY;
}

// =============================================================================
//
Axis Canvas::getRelativeZ() const
{
	const CameraInfo& camera = cameraInfo(this->camera());
	return static_cast<Axis>(3 - camera.localX - camera.localY);
}

// =============================================================================
//
void Canvas::setDepthValue (double depth)
{
	if (camera() < Camera::Free)
		m_depthValues[static_cast<int>(camera())] = depth;
}

// =============================================================================
//
double Canvas::getDepthValue() const
{
	if (camera() < Camera::Free)
		return m_depthValues[static_cast<int>(camera())];
	else
		return 0.0;
}

/*
 * This converts a 2D point on the screen to a 3D point in the model. If 'snap' is true, the 3D point will snap to the current grid.
 */
Vertex Canvas::convert2dTo3d(const QPoint& position2d, bool snap) const
{
	if (camera() == Camera::Free)
	{
		return {0, 0, 0};
	}
	else
	{
		Vertex position3d;
		const CameraInfo& camera = cameraInfo(this->camera());
		Axis axisX = camera.localX;
		Axis axisY = camera.localY;
		int signX = camera.negatedX ? -1 : 1;
		int signY = camera.negatedY ? -1 : 1;

		// Calculate cx and cy - these are the LDraw unit coords the cursor is at.
		double cx = (-virtualWidth() + ((2 * position2d.x() * virtualWidth()) / width()) - panning(X));
		double cy = (virtualHeight() - ((2 * position2d.y() * virtualHeight()) / height()) - panning(Y));

		if (snap)
		{
			cx = grid()->snap(cx, Grid::Coordinate);
			cy = grid()->snap(cy, Grid::Coordinate);
		}

		cx *= signX;
		cy *= signY;
		roundToDecimals(cx, 4);
		roundToDecimals(cy, 4);

		// Create the vertex from the coordinates
		position3d.setCoordinate(axisX, cx);
		position3d.setCoordinate(axisY, cy);
		position3d.setCoordinate(static_cast<Axis>(3 - axisX - axisY), getDepthValue());
		return position3d;
	}
}

/*
 * Inverse operation for the above - convert a 3D position to a 2D screen position.
 */
QPoint Canvas::convert3dTo2d(const Vertex& position3d) const
{
	if (camera() == Camera::Free)
	{
		return {0, 0};
	}
	else
	{
		const CameraInfo& camera = cameraInfo(this->camera());
		Axis axisX = camera.localX;
		Axis axisY = camera.localY;
		int signX = camera.negatedX ? -1 : 1;
		int signY = camera.negatedY ? -1 : 1;
		int rx = (((position3d[axisX] * signX) + virtualWidth() + panning(X)) * width()) / (2 * virtualWidth());
		int ry = (((position3d[axisY] * signY) - virtualHeight() + panning(Y)) * height()) / (2 * virtualHeight());
		return {rx, -ry};
	}
}

void Canvas::contextMenuEvent(QContextMenuEvent* event)
{
	m_window->spawnContextMenu(event->globalPos());
}

void Canvas::dragEnterEvent(QDragEnterEvent* event)
{
	if (m_window and event->source() == m_window->getPrimitivesTree() and m_window->getPrimitivesTree()->currentItem())
		event->acceptProposedAction();
}

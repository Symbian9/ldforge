/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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
#include "grid.h"
#include "lddocument.h"
#include "primitives.h"
#include "algorithms/geometry.h"
#include "generics/ring.h"

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

	if (camera() != Camera::Free)
	{
		// Paint the coordinates onto the screen.
		Vertex idealized = currentCamera().idealize(m_position3D);
		QString text = format(tr("X: %1, Y: %2, Z: %3, %4"), m_position3D[X], m_position3D[Y], m_position3D[Z],
		                      idealized.toString(true));
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
		painter.drawText(renderPoint, format("△ %1", largeNumberRep(m_document.triangleCount())));
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

/*
 * Assuming we're currently viewing from a fixed camera, draw a backdrop into it. Currently this means drawing the grid.
 */
void Canvas::drawFixedCameraBackdrop()
{
	// Find the top left corner of the grid
	Vertex topLeft = currentCamera().idealize(currentCamera().convert2dTo3d({0, 0}));
	Vertex bottomRight = currentCamera().idealize(currentCamera().convert2dTo3d({width(), height()}));
	qreal gridSize = grid()->coordinateSnap();

	if (config::useLineStipple())
		glEnable(GL_LINE_STIPPLE);

	glBegin(GL_LINES);

	switch (grid()->type())
	{
	case Grid::Cartesian:
		{
			qreal x0 = sign(topLeft.x) * (abs(topLeft.x) - fmod(abs(topLeft.x), gridSize));
			qreal y0 = sign(topLeft.y) * (abs(topLeft.y) - fmod(abs(topLeft.y), gridSize));

			static const auto prepareGridLine = [](qreal value) -> bool
			{
				if (not isZero(value))
				{
					if (isZero(fmod(value, 10.0)))
						glColor4f(0, 0, 0, 0.6);
					else
						glColor4f(0, 0, 0, 0.25);

					return true;
				}
				else
				{
					return false;
				}
			};

			for (qreal x = x0; x < bottomRight.x; x += gridSize)
			{
				if (prepareGridLine(x))
				{
					glVertex(currentCamera().realize({x, -10000, 999}));
					glVertex(currentCamera().realize({x, 10000, 999}));
				}
			}

			for (qreal y = y0; y < bottomRight.y; y += gridSize)
			{
				if (prepareGridLine(y))
				{
					glVertex(currentCamera().realize({-10000, y, 999}));
					glVertex(currentCamera().realize({10000, y, 999}));
				}
			}
		}
		break;

	case Grid::Polar:
		{
			const QPointF pole = grid()->pole();
			const qreal size = grid()->coordinateSnap();
			Vertex topLeft = currentCamera().idealize(currentCamera().convert2dTo3d({0, 0}));
			Vertex bottomRight = currentCamera().idealize(currentCamera().convert2dTo3d({width(), height()}));
			QPointF topLeft2d {topLeft.x, topLeft.y};
			QPointF bottomLeft2d {topLeft.x, bottomRight.y};
			QPointF bottomRight2d {bottomRight.x, bottomRight.y};
			QPointF topRight2d {bottomRight.x, topLeft.y};
			qreal smallestRadius = distanceFromPointToRectangle(pole, QRectF{topLeft2d, bottomRight2d});
			qreal largestRadius = max(QLineF {topLeft2d, pole}.length(),
			                          QLineF {bottomLeft2d, pole}.length(),
			                          QLineF {bottomRight2d, pole}.length(),
			                          QLineF {topRight2d, pole}.length());

			// Snap the radii to the grid.
			smallestRadius = round(smallestRadius / size) * size;
			largestRadius = round(largestRadius / size) * size;

			// Is the pole at (0, 0)? If so, then don't render the polar axes above the real ones.
			bool poleIsOrigin = isZero(pole.x()) and isZero(pole.y());
			glColor4f(0, 0, 0, 0.25);

			// Render the axes
			for (int i = 0; i < grid()->polarDivisions() / 2; ++i)
			{
				qreal azimuth = (2.0 * pi) * i / grid()->polarDivisions();

				if (not poleIsOrigin or not isZero(fmod(azimuth, pi / 2)))
				{
					QPointF extremum = {cos(azimuth) * 10000, sin(azimuth) * 10000};
					QPointF A = pole + extremum;
					QPointF B = pole - extremum;
					glVertex(currentCamera().realize({A.x(), A.y(), 999}));
					glVertex(currentCamera().realize({B.x(), B.y(), 999}));
				}
			}

			for (qreal radius = smallestRadius; radius <= largestRadius; radius += size)
			{
				if (not isZero(radius))
				{
					Vertex points[48];

					for (int i = 0; i < grid()->polarDivisions(); ++i)
					{
						qreal azimuth = (2.0 * pi) * i / grid()->polarDivisions();
						QPointF point = pole + QPointF {radius * cos(azimuth), radius * sin(azimuth)};
						points[i] = currentCamera().realize({point.x(), point.y(), 999});
					}

					for (int i = 0; i < grid()->polarDivisions(); ++i)
					{
						glVertex(points[i]);
						glVertex(ring(points, grid()->polarDivisions())[i + 1]);
					}
				}
			}
		}
		break;
	}

	glEnd();
	glDisable(GL_LINE_STIPPLE);

	if (this->camera() < Camera::Free)
	{
		GLfloat cullz = this->cullValues[static_cast<int>(this->camera())];
		GLRotationMatrix matrix = {
			1, 0, 0, cullz,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1,
		};
		glMultMatrixf(matrix);
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
	/*
	if (m_window and event->source() == m_window->getPrimitivesTree())
	{
		PrimitiveTreeItem* item = static_cast<PrimitiveTreeItem*> (m_window->getPrimitivesTree()->currentItem());
		QString primitiveName = item->primitive()->name;
		int position = m_window->suggestInsertPoint();
		currentDocument()->emplaceAt<LDSubfileReference>(
			position,
			primitiveName,
			Matrix {},
			Vertex {});
		m_window->select(currentDocument()->index(position));
		update();
		event->acceptProposedAction();
	}
	*/
}

void Canvas::keyReleaseEvent(QKeyEvent* event)
{
	m_currentEditMode->keyReleased(event);
	GLRenderer::keyReleaseEvent(event);
}

void Canvas::mouseMoveEvent(QMouseEvent* event)
{
	// Calculate 3d position of the cursor
	m_position3D = currentCamera().convert2dTo3d(mousePosition(), grid());

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
	drawBlipCoordinates(painter, pos3d, currentCamera().convert3dTo2d(pos3d));
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
	return currentCamera().isAxisNegated(Z) ? -1 : 1;
}

// =============================================================================
//
void Canvas::getRelativeAxes(Axis& relativeX, Axis& relativeY) const
{
	relativeX = currentCamera().axisX();
	relativeY = currentCamera().axisY();
}

// =============================================================================
//
Axis Canvas::getRelativeZ() const
{
	return currentCamera().axisZ();
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

void Canvas::contextMenuEvent(QContextMenuEvent* event)
{
	m_window->spawnContextMenu(event->globalPos());
}

void Canvas::dragEnterEvent(QDragEnterEvent* event)
{
	/*
	if (m_window and event->source() == m_window->getPrimitivesTree() and m_window->getPrimitivesTree()->currentItem())
		event->acceptProposedAction();
	*/
}

double Canvas::currentCullValue() const
{
	if (this->camera() < Camera::Free)
		return far - this->cullValues[static_cast<int>(this->camera())];
	else
		return 0.0;
}

void Canvas::setCullValue(double value)
{
	if (this->camera() < Camera::Free)
		this->cullValues[static_cast<int>(this->camera())] = far - value;
}

void Canvas::clearCurrentCullValue()
{
	if (this->camera() < Camera::Free)
		this->cullValues[static_cast<int>(this->camera())] = 0.0;
}

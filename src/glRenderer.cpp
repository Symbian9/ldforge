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

#define GL_GLEXT_PROTOTYPES
#include <GL/glu.h>
#include <GL/glext.h>
#include <QContextMenuEvent>
#include <QToolTip>
#include <QTimer>
#include <GL/glu.h>
#include "main.h"
#include "ldDocument.h"
#include "glRenderer.h"
#include "colors.h"
#include "mainwindow.h"
#include "miscallenous.h"
#include "editHistory.h"
#include "dialogs.h"
#include "messageLog.h"
#include "glCompiler.h"
#include "primitives.h"
#include "documentmanager.h"
#include "grid.h"

const CameraInfo g_cameraInfo[EnumLimits<Camera>::Count] =
{
	{{  1,  0, 0 }, X, Z, false, false, false }, // top
	{{  0,  0, 0 }, X, Y, false,  true, false }, // front
	{{  0,  1, 0 }, Z, Y,  true,  true, false }, // left
	{{ -1,  0, 0 }, X, Z, false,  true, true }, // bottom
	{{  0,  0, 0 }, X, Y,  true,  true, true }, // back
	{{  0, -1, 0 }, Z, Y, false,  true, true }, // right
	{{  1,  0, 0 }, X, Z, false, false, false }, // free (defensive dummy data)
};

ConfigOption (QColor BackgroundColor = "#FFFFFF")
ConfigOption (QColor MainColor = "#A0A0A0")
ConfigOption (float MainColorAlpha = 1.0)
ConfigOption (int LineThickness = 2)
ConfigOption (bool BfcRedGreenView = false)
ConfigOption (int Camera = 6)
ConfigOption (bool BlackEdges = false)
ConfigOption (bool DrawAxes = false)
ConfigOption (bool DrawWireframe = false)
ConfigOption (bool UseLogoStuds = false)
ConfigOption (bool AntiAliasedLines = true)
ConfigOption (bool RandomColors = false)
ConfigOption (bool HighlightObjectBelowCursor = true)
ConfigOption (bool DrawSurfaces = true)
ConfigOption (bool DrawEdgeLines = true)
ConfigOption (bool DrawConditionalLines = true)

// =============================================================================
//
GLRenderer::GLRenderer(LDDocument* document, QWidget* parent) :
    QGLWidget {parent},
    HierarchyElement {parent},
    m_document {document}
{
	m_camera = (Camera) m_config->camera();
	m_currentEditMode = AbstractEditMode::createByType (this, EditModeType::Select);
	m_compiler = new GLCompiler (this);
	m_toolTipTimer = new QTimer (this);
	m_toolTipTimer->setSingleShot (true);
	m_thinBorderPen = QPen (QColor (0, 0, 0, 208), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	m_thinBorderPen.setWidth (1);
	setAcceptDrops (true);
	connect (m_toolTipTimer, SIGNAL (timeout()), this, SLOT (slot_toolTipTimer()));
	initOverlaysFromObjects();

	if (not currentDocumentData().init)
	{
		resetAllAngles();
		currentDocumentData().init = true;
	}

	currentDocumentData().needZoomToFit = true;

	// Init camera icons
	for (Camera camera : iterateEnum<Camera>())
	{
		const char* cameraIconNames[EnumLimits<Camera>::Count] =
		{
			"camera-top", "camera-front", "camera-left",
			"camera-bottom", "camera-back", "camera-right",
			"camera-free"
		};

		CameraIcon* info = &m_cameraIcons[camera];
		info->image = GetIcon (cameraIconNames[camera]);
		info->camera = camera;
	}

	calcCameraIcons();
}

// =============================================================================
//
GLRenderer::~GLRenderer()
{
	m_compiler->setRenderer (nullptr);
	delete m_compiler;
	delete m_currentEditMode;
	glDeleteBuffers (1, &m_axesVbo);
	glDeleteBuffers (1, &m_axesColorVbo);
}

// =============================================================================
// Calculates the "hitboxes" of the camera icons so that we can tell when the
// cursor is pointing at the camera icon.
//
void GLRenderer::calcCameraIcons()
{
	int i = 0;

	for (CameraIcon& info : m_cameraIcons)
	{
		// MATH
		int x1 = (m_width - (info.camera != FreeCamera ? 48 : 16)) + ((i % 3) * 16) - 1;
		int y1 = ((i / 3) * 16) + 1;

		info.sourceRect = QRect (0, 0, 16, 16);
		info.targetRect = QRect (x1, y1, 16, 16);
		info.hitRect = QRect (
			info.targetRect.x(),
			info.targetRect.y(),
			info.targetRect.width() + 1,
			info.targetRect.height() + 1
		);

		++i;
	}
}

// =============================================================================
//
void GLRenderer::initGLData()
{
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1.0f, 1.0f);
	glEnable (GL_DEPTH_TEST);
	glShadeModel (GL_SMOOTH);
	glEnable (GL_MULTISAMPLE);

	if (m_config->antiAliasedLines())
	{
		glEnable (GL_LINE_SMOOTH);
		glEnable (GL_POLYGON_SMOOTH);
		glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint (GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	}
	else
	{
		glDisable (GL_LINE_SMOOTH);
		glDisable (GL_POLYGON_SMOOTH);
	}
}

bool GLRenderer::isDrawOnly() const
{
	return m_isDrawOnly;
}

void GLRenderer::setDrawOnly (bool value)
{
	m_isDrawOnly = value;
}

LDDocument* GLRenderer::document() const
{
	return m_document;
}

GLCompiler* GLRenderer::compiler() const
{
	return m_compiler;
}

LDObject* GLRenderer::objectAtCursor() const
{
	return m_objectAtCursor;
}

// =============================================================================
//
void GLRenderer::needZoomToFit()
{
	if (document())
		currentDocumentData().needZoomToFit = true;
}

// =============================================================================
//
void GLRenderer::resetAngles()
{
	if (m_initialized)
	{
		// Why did I even bother trying to compute this by pen and paper? Let GL figure it out...
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glRotatef(30, 1, 0, 0);
		glRotatef(330, 0, 1, 0);
		glGetFloatv(GL_MODELVIEW_MATRIX, currentDocumentData().rotationMatrix.data());
		glPopMatrix();
	}
	panning(X) = panning(Y) = 0.0f;
	needZoomToFit();
}

// =============================================================================
//
void GLRenderer::resetAllAngles()
{
	Camera oldcam = camera();

	for (int i = 0; i < 7; ++i)
	{
		setCamera ((Camera) i);
		resetAngles();
	}

	setCamera (oldcam);
}

// =============================================================================
//
void GLRenderer::initializeGL()
{
	initializeOpenGLFunctions();
	setBackground();
	glLineWidth (m_config->lineThickness());
	glLineStipple (1, 0x6666);
	setAutoFillBackground (false);
	setMouseTracking (true);
	setFocusPolicy (Qt::WheelFocus);
	compiler()->initialize();
	initializeAxes();
	m_initialized = true;
	// Now that GL is initialized, we can reset angles.
	resetAllAngles();
}

// =============================================================================
//
void GLRenderer::initializeAxes()
{
	// Definitions for visual axes, drawn on the screen
	struct
	{
		QColor color;
		Vertex extrema;
	} axisInfo[3] =
	{
	    { QColor {192,  96,  96}, Vertex {10000, 0, 0} }, // X
	    { QColor {48,  192,  48}, Vertex {0, 10000, 0} }, // Y
		{ QColor {48,  112, 192}, Vertex {0, 0, 10000} }, // Z
	};

	float axisdata[18];
	float colorData[18];
	memset (axisdata, 0, sizeof axisdata);

	for (int i = 0; i < 3; ++i)
	{
		const auto& data = axisInfo[i];

		for (Axis axis : axes)
		{
			axisdata[(i * 6) + axis] = data.extrema[axis];
			axisdata[(i * 6) + 3 + axis] = -data.extrema[axis];
		}

		int offset = i * 6;
		colorData[offset + 0] = colorData[offset + 3] = data.color.red();
		colorData[offset + 1] = colorData[offset + 4] = data.color.green();
		colorData[offset + 2] = colorData[offset + 5] = data.color.blue();
	}

	glGenBuffers (1, &m_axesVbo);
	glBindBuffer (GL_ARRAY_BUFFER, m_axesVbo);
	glBufferData (GL_ARRAY_BUFFER, sizeof axisdata, axisdata, GL_STATIC_DRAW);
	glGenBuffers (1, &m_axesColorVbo);
	glBindBuffer (GL_ARRAY_BUFFER, m_axesColorVbo);
	glBufferData (GL_ARRAY_BUFFER, sizeof colorData, colorData, GL_STATIC_DRAW);
	glBindBuffer (GL_ARRAY_BUFFER, 0);
}

// =============================================================================
//
void GLRenderer::setBackground()
{
	if (not m_isDrawingSelectionScene)
	{
		// Otherwise use the background that the user wants.
		QColor color = m_config->backgroundColor();

		if (color.isValid())
		{
			color.setAlpha(255);
			m_useDarkBackground = luma(color) < 80;
			m_backgroundColor = color;
			qglClearColor(color);
		}
	}
	else
	{
		// The picking scene requires a black background.
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	}
}

// =============================================================================
//
void GLRenderer::refresh()
{
	update();

	if (isVisible())
		swapBuffers();
}

// =============================================================================
//
void GLRenderer::hardRefresh()
{
	if (m_initialized)
	{
		compiler()->compileDocument (currentDocument());
		refresh();
	}
}

// =============================================================================
//
void GLRenderer::resizeGL (int w, int h)
{
	m_width = w;
	m_height = h;
	calcCameraIcons();
	glViewport (0, 0, w, h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluPerspective (45.0f, (double) w / (double) h, 1.0f, 10000.0f);
	glMatrixMode (GL_MODELVIEW);
}

// =============================================================================
//
void GLRenderer::drawGLScene()
{
	if (document() == nullptr)
		return;

	if (currentDocumentData().needZoomToFit)
	{
		currentDocumentData().needZoomToFit = false;
		zoomAllToFit();
	}

	if (m_config->drawWireframe() and not m_isDrawingSelectionScene)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (camera() != FreeCamera)
	{
		glMatrixMode (GL_PROJECTION);
		glPushMatrix();

		glLoadIdentity();
		glOrtho (-m_virtualWidth, m_virtualWidth, -m_virtualHeight, m_virtualHeight, -100.0f, 100.0f);
		glTranslatef(panning (X), panning (Y), 0.0f);

		if (camera() != FrontCamera and camera() != BackCamera)
		{
			glRotatef(90.0f, g_cameraInfo[camera()].glrotate[0],
				g_cameraInfo[camera()].glrotate[1],
				g_cameraInfo[camera()].glrotate[2]);
		}

		// Back camera needs to be handled differently
		if (camera() == BackCamera)
		{
			glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
			glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
		}
	}
	else
	{
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, -2.0f);
		glTranslatef(panning (X), panning (Y), -zoom());
		glMultMatrixf(currentDocumentData().rotationMatrix.constData());
	}

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);

	if (m_isDrawingSelectionScene)
	{
		drawVbos (TrianglesVbo, PickColorsVboComplement, GL_TRIANGLES);
		drawVbos (QuadsVbo, PickColorsVboComplement, GL_QUADS);
		drawVbos (LinesVbo, PickColorsVboComplement, GL_LINES);
		drawVbos (ConditionalLinesVbo, PickColorsVboComplement, GL_LINES);
	}
	else
	{
		if (m_config->bfcRedGreenView())
		{
			glEnable (GL_CULL_FACE);
			glCullFace (GL_BACK);
			drawVbos (TrianglesVbo, BfcFrontColorsVboComplement, GL_TRIANGLES);
			drawVbos (QuadsVbo, BfcFrontColorsVboComplement, GL_QUADS);
			glCullFace (GL_FRONT);
			drawVbos (TrianglesVbo, BfcBackColorsVboComplement, GL_TRIANGLES);
			drawVbos (QuadsVbo, BfcBackColorsVboComplement, GL_QUADS);
			glDisable (GL_CULL_FACE);
		}
		else
		{
			ComplementVboType colors;

			if (m_config->randomColors())
				colors = RandomColorsVboComplement;
			else
				colors = NormalColorsVboComplement;

			drawVbos (TrianglesVbo, colors, GL_TRIANGLES);
			drawVbos (QuadsVbo, colors, GL_QUADS);
		}

		drawVbos (LinesVbo, NormalColorsVboComplement, GL_LINES);
		glEnable (GL_LINE_STIPPLE);
		drawVbos (ConditionalLinesVbo, NormalColorsVboComplement, GL_LINES);
		glDisable (GL_LINE_STIPPLE);

		if (m_config->drawAxes())
		{
			glBindBuffer (GL_ARRAY_BUFFER, m_axesVbo);
			glVertexPointer (3, GL_FLOAT, 0, NULL);
			glBindBuffer (GL_ARRAY_BUFFER, m_axesVbo);
			glColorPointer (3, GL_FLOAT, 0, NULL);
			glDrawArrays (GL_LINES, 0, 6);
			CHECK_GL_ERROR();
		}
	}

	glPopMatrix();
	glBindBuffer (GL_ARRAY_BUFFER, 0);
	glDisableClientState (GL_VERTEX_ARRAY);
	glDisableClientState (GL_COLOR_ARRAY);
	CHECK_GL_ERROR();
	glDisable (GL_CULL_FACE);
	glMatrixMode (GL_MODELVIEW);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
}

// =============================================================================
//
void GLRenderer::drawVbos (SurfaceVboType surface, ComplementVboType colors, GLenum type)
{
	// Filter this through some configuration options
	if ((isOneOf (surface, QuadsVbo, TrianglesVbo) and m_config->drawSurfaces() == false)
		or (surface == LinesVbo and m_config->drawEdgeLines() == false)
		or (surface == ConditionalLinesVbo and m_config->drawConditionalLines() == false))
	{
		return;
	}

	int surfaceVboNumber = m_compiler->vboNumber(surface, SurfacesVboComplement);
	int colorVboNumber = m_compiler->vboNumber(surface, colors);
	m_compiler->prepareVBO(surfaceVboNumber);
	m_compiler->prepareVBO(colorVboNumber);
	GLuint surfaceVbo = m_compiler->vbo(surfaceVboNumber);
	GLuint colorVbo = m_compiler->vbo(colorVboNumber);
	GLsizei count = m_compiler->vboSize(surfaceVboNumber) / 3;

	if (count > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, surfaceVbo);
		glVertexPointer(3, GL_FLOAT, 0, nullptr);
		CHECK_GL_ERROR();
		glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
		glColorPointer(4, GL_FLOAT, 0, nullptr);
		CHECK_GL_ERROR();
		glDrawArrays(type, 0, count);
		CHECK_GL_ERROR();
	}
}

// =============================================================================
//
// This converts a 2D point on the screen to a 3D point in the model. If 'snap'
// is true, the 3D point will snap to the current grid.
//
Vertex GLRenderer::convert2dTo3d (const QPoint& position2d, bool snap) const
{
	if (camera() == FreeCamera)
	{
		return {0, 0, 0};
	}
	else
	{
		Vertex position3d;
		const CameraInfo* camera = &g_cameraInfo[this->camera()];
		Axis axisX = camera->localX;
		Axis axisY = camera->localY;
		int signX = camera->negatedX ? -1 : 1;
		int signY = camera->negatedY ? -1 : 1;

		// Calculate cx and cy - these are the LDraw unit coords the cursor is at.
		double cx = (-m_virtualWidth + ((2 * position2d.x() * m_virtualWidth) / m_width) - panning(X));
		double cy = (m_virtualHeight - ((2 * position2d.y() * m_virtualHeight) / m_height) - panning(Y));

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
QPoint GLRenderer::convert3dTo2d(const Vertex& position3d) const
{
	if (camera() == FreeCamera)
	{
		return {0, 0};
	}
	else
	{
		const CameraInfo* camera = &g_cameraInfo[this->camera()];
		Axis axisX = camera->localX;
		Axis axisY = camera->localY;
		int signX = camera->negatedX ? -1 : 1;
		int signY = camera->negatedY ? -1 : 1;
		int rx = (((position3d[axisX] * signX) + m_virtualWidth + panning(X)) * m_width) / (2 * m_virtualWidth);
		int ry = (((position3d[axisY] * signY) - m_virtualHeight + panning(Y)) * m_height) / (2 * m_virtualHeight);
		return {rx, -ry};
	}
}

QPen GLRenderer::textPen() const
{
	return {m_useDarkBackground ? Qt::white : Qt::black};
}

QPen GLRenderer::linePen() const
{
	QPen linepen = m_thinBorderPen;
	linepen.setWidth(2);
	linepen.setColor(luma(m_backgroundColor) < 40 ? Qt::white : Qt::black);
	return linepen;
}

// =============================================================================
//
void GLRenderer::paintEvent(QPaintEvent*)
{
	makeCurrent();
	m_virtualWidth = zoom();
	m_virtualHeight = (m_height * m_virtualWidth) / m_width;
	initGLData();
	drawGLScene();
	QPainter painter {this};
	QFontMetrics metrics {QFont {}};
	painter.setRenderHint(QPainter::Antialiasing);

	// If we wish to only draw the brick, stop here
	if (isDrawOnly() or m_isDrawingSelectionScene)
		return;

#ifndef RELEASE
	{
		QString text = format("Rotation: %1\nPanning: (%2, %3), Zoom: %4",
		    currentDocumentData().rotationMatrix, panning(X), panning(Y), zoom());
		QRect textSize = metrics.boundingRect(0, 0, m_width, m_height, Qt::AlignCenter, text);
		painter.setPen(textPen());
		painter.drawText((width() - textSize.width()) / 2, height() - textSize.height(), textSize.width(),
			textSize.height(), Qt::AlignCenter, text);
	}
#endif

	if (camera() != FreeCamera)
	{
		// Paint the overlay image if we have one
		const LDGLOverlay& overlay = currentDocumentData().overlays[camera()];

		if (overlay.image)
		{
			QPoint v0 = convert3dTo2d(currentDocumentData().overlays[camera()].v0);
			QPoint v1 = convert3dTo2d(currentDocumentData().overlays[camera()].v1);
			QRect targetRect = {v0.x(), v0.y(), qAbs(v1.x() - v0.x()), qAbs(v1.y() - v0.y())};
			QRect sourceRect = {0, 0, overlay.image->width(), overlay.image->height()};
			painter.drawImage(targetRect, *overlay.image, sourceRect);
		}

		// Paint the coordinates onto the screen.
		QString text = format(tr("X: %1, Y: %2, Z: %3"), m_position3D[X], m_position3D[Y], m_position3D[Z]);
		QFontMetrics metrics {font()};
		QRect textSize = metrics.boundingRect (0, 0, m_width, m_height, Qt::AlignCenter, text);
		painter.setPen(textPen());
		painter.drawText(m_width - textSize.width(), m_height - 16,
		                 textSize.width(), textSize.height(), Qt::AlignCenter, text);
	}

	{
		// Draw edit mode HUD
		m_currentEditMode->render(painter);

		// Draw a background for the selected camera
		painter.setPen(m_thinBorderPen);
		painter.setBrush(QBrush {QColor {0, 128, 160, 128}});
		painter.drawRect(m_cameraIcons[camera()].hitRect);

		// Draw the camera icons
		for (CameraIcon& info : m_cameraIcons)
		{
			// Don't draw the free camera icon when we can't use the free camera
			if (&info == &m_cameraIcons[FreeCamera] and not m_currentEditMode->allowFreeCamera())
				continue;

			painter.drawPixmap(info.targetRect, info.image, info.sourceRect);
		}

		// Draw a label for the current camera in the bottom left corner
		{
			int margin = 4;
			painter.setPen(textPen());
			painter.drawText(QPoint {margin, height() - margin - metrics.descent()}, currentCameraName());

			// Also render triangle count.
			if (m_document)
			{
				QPoint renderPoint = {margin, height() - margin - metrics.height() - metrics.descent()};
				painter.drawText(renderPoint, format("△ %1", m_document->triangleCount()));
			}
		}

		// Tool tips
		if (m_drawToolTip)
		{
			if (not m_cameraIcons[m_toolTipCamera].targetRect.contains (m_mousePosition))
				m_drawToolTip = false;
			else
				QToolTip::showText(m_globalpos, currentCameraName());
		}
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

// =============================================================================
//
void GLRenderer::drawPoint(QPainter& painter, QPointF pos, QColor color) const
{
	int pointSize = 8;
	QPen pen = m_thinBorderPen;
	pen.setWidth(1);
	painter.setPen(pen);
	painter.setBrush(color);
	painter.drawEllipse(pos.x() - pointSize / 2, pos.y() - pointSize / 2, pointSize, pointSize);
}

void GLRenderer::drawBlipCoordinates(QPainter& painter, const Vertex& pos3d)
{
	drawBlipCoordinates (painter, pos3d, convert3dTo2d (pos3d));
}

void GLRenderer::drawBlipCoordinates(QPainter& painter, const Vertex& pos3d, QPointF pos)
{
	painter.setPen (textPen());
	painter.drawText (pos.x(), pos.y() - 8, pos3d.toString (true));
}

// =============================================================================
//
void GLRenderer::mouseReleaseEvent(QMouseEvent* ev)
{
	bool wasLeft = (m_lastButtons & Qt::LeftButton) and not (ev->buttons() & Qt::LeftButton);
	Qt::MouseButtons releasedbuttons = m_lastButtons & ~ev->buttons();
	m_panning = false;

	if (wasLeft)
	{
		// Check if we selected a camera icon
		if (not mouseHasMoved())
		{
			for (CameraIcon& info : m_cameraIcons)
			{
				if (info.targetRect.contains (ev->pos()))
				{
					setCamera (info.camera);
					goto end;
				}
			}
		}
	}

	if (not isDrawOnly())
	{
		AbstractEditMode::MouseEventData data;
		data.ev = ev;
		data.mouseMoved = mouseHasMoved();
		data.keymods = m_currentKeyboardModifiers;
		data.releasedButtons = releasedbuttons;

		if (m_currentEditMode->mouseReleased (data))
			goto end;
	}

end:
	update();
	m_totalMouseMove = 0;
}

// =============================================================================
//
void GLRenderer::mousePressEvent(QMouseEvent* event)
{
	m_totalMouseMove = 0;
	m_lastButtons = event->buttons();

	if (m_currentEditMode->mousePressed(event))
		event->accept();
}

// =============================================================================
//
void GLRenderer::mouseMoveEvent(QMouseEvent* event)
{
	int xMove = event->x() - m_mousePosition.x();
	int yMove = event->y() - m_mousePosition.y();
	m_totalMouseMove += qAbs(xMove) + qAbs(yMove);
	m_isCameraMoving = false;

	if (not m_currentEditMode->mouseMoved (event))
	{
		bool left = event->buttons() & Qt::LeftButton;
		bool mid = event->buttons() & Qt::MidButton;
		bool shift = event->modifiers() & Qt::ShiftModifier;

		if (mid or (left and shift))
		{
			panning(X) += 0.03f * xMove * (zoom() / 7.5f);
			panning(Y) -= 0.03f * yMove * (zoom() / 7.5f);
			m_panning = true;
			m_isCameraMoving = true;
		}
		else if (left and camera() == FreeCamera and (xMove != 0 or yMove != 0))
		{
			// Apply current rotation input to the rotation matrix
			// ref: https://forums.ldraw.org/thread-22006-post-24426.html#pid24426
			glPushMatrix();
			glLoadIdentity();
			// 0.6 is an arbitrary rotation sensitivity scalar
			glRotatef(0.6 * hypot(xMove, yMove), yMove, xMove, 0);
			glMultMatrixf(currentDocumentData().rotationMatrix.constData());
			glGetFloatv(GL_MODELVIEW_MATRIX, currentDocumentData().rotationMatrix.data());
			glPopMatrix();
			m_isCameraMoving = true;
		}
	}

	// Start the tool tip timer
	if (not m_drawToolTip)
		m_toolTipTimer->start (500);

	// Update 2d position
	m_mousePosition = event->pos();
	m_globalpos = event->globalPos();
	m_mousePositionF = event->localPos();

	// Calculate 3d position of the cursor
	m_position3D = (camera() != FreeCamera) ? convert2dTo3d (m_mousePosition, true) : Origin;

	highlightCursorObject();
	update();
	event->accept();
}

// =============================================================================
//
void GLRenderer::keyPressEvent(QKeyEvent* event)
{
	m_currentKeyboardModifiers = event->modifiers();
}

// =============================================================================
//
void GLRenderer::keyReleaseEvent(QKeyEvent* event)
{
	m_currentKeyboardModifiers = event->modifiers();
	m_currentEditMode->keyReleased(event);
	update();
}

// =============================================================================
//
void GLRenderer::wheelEvent(QWheelEvent* ev)
{
	makeCurrent();
	zoomNotch(ev->delta() > 0);
	zoom() = qBound(0.01, zoom(), 10000.0);
	m_isCameraMoving = true;
	update();
	ev->accept();
}

// =============================================================================
//
void GLRenderer::leaveEvent(QEvent*)
{
	m_drawToolTip = false;
	m_toolTipTimer->stop();
	update();
}

// =============================================================================
//
void GLRenderer::contextMenuEvent(QContextMenuEvent* event)
{
	m_window->spawnContextMenu(event->globalPos());
}

// =============================================================================
//
void GLRenderer::setCamera(Camera camera)
{
	// The edit mode may forbid the free camera.
	if (m_currentEditMode->allowFreeCamera() or camera != FreeCamera)
	{
		m_camera = camera;
		m_config->setCamera(int {camera});
	}
}

// =============================================================================
//
void GLRenderer::pick (int mouseX, int mouseY, bool additive)
{
	pick (QRect (mouseX, mouseY, mouseX + 1, mouseY + 1), additive);
}

// =============================================================================
//
void GLRenderer::pick(const QRect& range, bool additive)
{
	makeCurrent();
	QSet<LDObject*> priorSelection = selectedObjects();
	QSet<LDObject*> newSelection;

	// If we're doing an additive selection, we start off with the existing selection.
	// Otherwise we start selecting from scratch.
	if (additive)
		newSelection = priorSelection;

	// Paint the picking scene
	setPicking(true);
	drawGLScene();

	int x0 = range.left();
	int y0 = range.top();
	int x1 = x0 + range.width();
	int y1 = y0 + range.height();

	// Clamp the values to ensure they're within bounds
	x0 = qMax (0, x0);
	y0 = qMax (0, y0);
	x1 = qMin (x1, m_width);
	y1 = qMin (y1, m_height);
	const int areawidth = (x1 - x0);
	const int areaheight = (y1 - y0);
	const qint32 numpixels = areawidth * areaheight;

	// Allocate space for the pixel data.
	QVector<unsigned char> pixelData;
	pixelData.resize(4 * numpixels);

	// Read pixels from the color buffer.
	glReadPixels(x0, m_height - y1, areawidth, areaheight, GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data());

	QSet<int32_t> indices;

	// Go through each pixel read and add them to the selection.
	// Each pixel maps to an LDObject index injectively.
	// Note: black is background, those indices are skipped.
	for (unsigned char *pixelCursor = pixelData.begin(); pixelCursor < pixelData.end(); pixelCursor += 4)
	{
		int32_t index = pixelCursor[0] * 0x10000 +
		                pixelCursor[1] * 0x100 +
		                pixelCursor[2] * 0x1;
		if (index != 0)
			indices.insert(index);
	}

	// For each index read, resolve the LDObject behind it and add it to the selection.
	for (int32_t index : indices)
	{
		LDObject* object = LDObject::fromID(index);

		if (object != nullptr)
		{
			// If this is an additive single pick and the object is currently selected,
			// we remove it from selection instead.
			if (additive and newSelection.contains(object))
				newSelection.remove(object);
			else
				newSelection.insert(object);
		}
	}

	// Select all objects that we now have selected that were not selected before.
	for (LDObject* object : newSelection - priorSelection)
	{
		m_document->addToSelection(object);
		compileObject(object);
	}

	// Likewise, deselect whatever was selected that isn't anymore.
	for (LDObject* object : priorSelection - newSelection)
	{
		m_document->removeFromSelection(object);
		compileObject(object);
	}

	m_window->updateSelection();
	setPicking(false);
	repaint();
}

//
// Simpler version of GLRenderer::pick which simply picks whatever object on the cursor
//
LDObject* GLRenderer::pickOneObject (int mouseX, int mouseY)
{
	unsigned char pixel[4];
	makeCurrent();
	setPicking(true);
	drawGLScene();
	glReadPixels(mouseX, m_height - mouseY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	LDObject* object = LDObject::fromID(pixel[0] * 0x10000 +
	                                    pixel[1] * 0x100 +
	                                    pixel[2] * 0x1);
	setPicking(false);
	repaint();
	return object;
}

// =============================================================================
//
void GLRenderer::setEditMode(EditModeType a)
{
	if (m_currentEditMode and m_currentEditMode->type() == a)
		return;

	delete m_currentEditMode;
	m_currentEditMode = AbstractEditMode::createByType(this, a);

	// If we cannot use the free camera, use the top one instead.
	if (camera() == FreeCamera and not m_currentEditMode->allowFreeCamera())
		setCamera(TopCamera);

	m_window->updateEditModeActions();
	update();
}

// =============================================================================
//
EditModeType GLRenderer::currentEditModeType() const
{
	return m_currentEditMode->type();
}

// =============================================================================
//
void GLRenderer::setPicking(bool value)
{
	m_isDrawingSelectionScene = value;
	setBackground();

	if (m_isDrawingSelectionScene)
	{
		glDisable(GL_DITHER);

		// Use particularly thick lines while picking ease up selecting lines.
		glLineWidth(qMax<double>(m_config->lineThickness(), 6.5));
	}
	else
	{
		glEnable(GL_DITHER);

		// Restore line thickness
		glLineWidth(m_config->lineThickness());
	}
}

// =============================================================================
//
void GLRenderer::getRelativeAxes(Axis& relativeX, Axis& relativeY) const
{
	const CameraInfo* camera = &g_cameraInfo[this->camera()];
	relativeX = camera->localX;
	relativeY = camera->localY;
}

// =============================================================================
//
Axis GLRenderer::getRelativeZ() const
{
	const CameraInfo* camera = &g_cameraInfo[this->camera()];
	return static_cast<Axis>(3 - camera->localX - camera->localY);
}

// =============================================================================
//
void GLRenderer::compileObject (LDObject* obj)
{
	compiler()->stageForCompilation (obj);
}

// =============================================================================
//
void GLRenderer::forgetObject(LDObject* obj)
{
	compiler()->dropObjectInfo(obj);
	compiler()->unstage(obj);

	if (m_objectAtCursor == obj)
		m_objectAtCursor = nullptr;
}

// =============================================================================
//
QByteArray GLRenderer::capturePixels()
{
	QByteArray result;
	result.resize (4 * width() * height());
	m_takingScreenCapture = true;
	update(); // Smile!
	m_takingScreenCapture = false;
	glReadPixels(0, 0, width(), height(), GL_RGBA, GL_UNSIGNED_BYTE, result.data());
	return result;
}

// =============================================================================
//
void GLRenderer::slot_toolTipTimer()
{
	// We come here if the cursor has stayed in one place for longer than a
	// a second. Check if we're holding it over a camera icon - if so, draw
	// a tooltip.
	for (CameraIcon & icon : m_cameraIcons)
	{
		if (icon.targetRect.contains (m_mousePosition))
		{
			m_toolTipCamera = icon.camera;
			m_drawToolTip = true;
			update();
			break;
		}
	}
}

// =============================================================================
//
Axis GLRenderer::getCameraAxis (bool y, Camera camid)
{
	if (camid == (Camera) -1)
		camid = camera();

	const CameraInfo* cam = &g_cameraInfo[camid];
	return (y) ? cam->localY : cam->localX;
}

// =============================================================================
//
bool GLRenderer::setupOverlay (Camera camera, QString fileName, int x, int y, int w, int h)
{
	QImage* image = new QImage (QImage (fileName).convertToFormat (QImage::Format_ARGB32));
	LDGLOverlay& info = getOverlay (camera);

	if (image->isNull())
	{
		Critical (tr ("Failed to load overlay image!"));
		currentDocumentData().overlays[camera].invalid = true;
		delete image;
		return false;
	}

	delete info.image; // delete the old image

	info.fileName = fileName;
	info.width = w;
	info.height = h;
	info.offsetX = x;
	info.offsetY = y;
	info.image = image;
	info.invalid = false;

	if (info.width == 0)
		info.width = (info.height * image->width()) / image->height();
	else if (info.height == 0)
		info.height = (info.width * image->height()) / image->width();

	Axis localX = getCameraAxis (false, camera);
	Axis localY = getCameraAxis (true, camera);
	int signX = g_cameraInfo[camera].negatedX ? -1 : 1;
	int signY = g_cameraInfo[camera].negatedY ? -1 : 1;

	info.v0 = info.v1 = Origin;
	info.v0.setCoordinate (localX, -(info.offsetX * info.width * signX) / image->width());
	info.v0.setCoordinate (localY, (info.offsetY * info.height * signY) / image->height());
	info.v1.setCoordinate (localX, info.v0[localX] + info.width);
	info.v1.setCoordinate (localY, info.v0[localY] + info.height);

	// Set alpha of all pixels to 0.5
	for (int i = 0; i < image->width(); ++i)
	for (int j = 0; j < image->height(); ++j)
	{
		uint32 pixel = image->pixel (i, j);
		image->setPixel (i, j, 0x80000000 | (pixel & 0x00FFFFFF));
	}

	updateOverlayObjects();
	return true;
}

// =============================================================================
//
void GLRenderer::clearOverlay()
{
	if (camera() == FreeCamera)
		return;

	LDGLOverlay& info = currentDocumentData().overlays[camera()];
	delete info.image;
	info.image = nullptr;

	updateOverlayObjects();
}

// =============================================================================
//
void GLRenderer::setDepthValue (double depth)
{
	if (camera() < FreeCamera)
		currentDocumentData().depthValues[camera()] = depth;
}

// =============================================================================
//
double GLRenderer::getDepthValue() const
{
	if (camera() < FreeCamera)
		return currentDocumentData().depthValues[camera()];
	else
		return 0.0;
}

// =============================================================================
//
QString GLRenderer::cameraName (Camera camera) const
{
	switch (camera)
	{
	case TopCamera: return tr ("Top Camera");
	case FrontCamera: return tr ("Front Camera");
	case LeftCamera: return tr ("Left Camera");
	case BottomCamera: return tr ("Bottom Camera");
	case BackCamera: return tr ("Back Camera");
	case RightCamera: return tr ("Right Camera");
	case FreeCamera: return tr ("Free Camera");
	default: break;
	}

	return "";
}

QString GLRenderer::currentCameraName() const
{
	return cameraName (camera());
}

// =============================================================================
//
LDGLOverlay& GLRenderer::getOverlay (int newcam)
{
	return currentDocumentData().overlays[newcam];
}

// =============================================================================
//
void GLRenderer::zoomNotch (bool inward)
{
	zoom() *= inward ? 0.833f : 1.2f;
}

// =============================================================================
//
void GLRenderer::zoomToFit()
{
	zoom() = 30.0f;

	if (document() == nullptr or m_width == -1 or m_height == -1)
		return;

	bool lastfilled = false;
	bool firstrun = true;
	enum { black = 0xFF000000 };
	bool inward = true;
	int runaway = 50;

	// Use the pick list while drawing the scene, this way we can tell whether borders
	// are background or not.
	setPicking (true);

	while (--runaway)
	{
		if (zoom() > 10000.0 or zoom() < 0.0)
		{
			// Nothing to draw if we get here.
			zoom() = 30.0;
			break;
		}

		zoomNotch (inward);
		QVector<unsigned char> capture (4 * m_width * m_height);
		drawGLScene();
		glReadPixels (0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, capture.data());
		QImage image (capture.constData(), m_width, m_height, QImage::Format_ARGB32);
		bool filled = false;

		// Check the top and bottom rows
		for (int i = 0; i < image.width(); ++i)
		{
			if (image.pixel (i, 0) != black or image.pixel (i, m_height - 1) != black)
			{
				filled = true;
				break;
			}
		}

		// Left and right edges
		if (filled == false)
		{
			for (int i = 0; i < image.height(); ++i)
			{
				if (image.pixel (0, i) != black or image.pixel (m_width - 1, i) != black)
				{
					filled = true;
					break;
				}
			}
		}

		if (firstrun)
		{
			// If this is the first run, we don't know enough to determine
			// whether the zoom was to fit, so we mark in our knowledge so
			// far and start over.
			inward = not filled;
			firstrun = false;
		}
		else
		{
			// If this run filled the screen and the last one did not, the
			// last run had ideal zoom - zoom a bit back and we should reach it.
			if (filled and not lastfilled)
			{
				zoomNotch (false);
				break;
			}

			// If this run did not fill the screen and the last one did, we've
			// now reached ideal zoom so we're done here.
			if (not filled and lastfilled)
				break;

			inward = not filled;
		}

		lastfilled = filled;
	}

	setPicking (false);
}

// =============================================================================
//
void GLRenderer::zoomAllToFit()
{
	zoomToFit();
}

// =============================================================================
//
void GLRenderer::mouseDoubleClickEvent (QMouseEvent* ev)
{
	if (m_currentEditMode->mouseDoubleClicked (ev))
		ev->accept();
}

// =============================================================================
//
LDOverlay* GLRenderer::findOverlayObject (Camera cam)
{
	for (LDObject* obj : document()->objects())
	{
		LDOverlay* overlay = dynamic_cast<LDOverlay*> (obj);

		if (overlay and overlay->camera() == cam)
			return overlay;
	}

	return nullptr;
}

// =============================================================================
//
// Read in overlays from the current file and update overlay info accordingly.
//
void GLRenderer::initOverlaysFromObjects()
{
	for (Camera camera : iterateEnum<Camera>())
	{
		if (camera == FreeCamera)
			continue;

		LDGLOverlay& meta = currentDocumentData().overlays[camera];
		LDOverlay* overlay = findOverlayObject (camera);

		if (overlay == nullptr and meta.image)
		{
			// The document doesn't have an overlay for this camera but we have an image for it, delete the image.
			delete meta.image;
			meta.image = nullptr;
		}
		else if (overlay
			and (meta.image == nullptr or meta.fileName != overlay->fileName())
			and not meta.invalid)
		{
			// Found a valid overlay definition for this camera, set it up for use.
			setupOverlay (camera, overlay->fileName(), overlay->x(),
				overlay->y(), overlay->width(), overlay->height());
		}
	}
}

// =============================================================================
//
void GLRenderer::updateOverlayObjects()
{
	for (Camera camera : iterateEnum<Camera>())
	{
		if (camera == FreeCamera)
			continue;

		LDGLOverlay& meta = currentDocumentData().overlays[camera];
		LDOverlay* overlayObject = findOverlayObject (camera);

		if (meta.image == nullptr and overlayObject)
		{
			// If this is the last overlay image, we need to remove the empty space after it as well.
			LDObject* nextobj = overlayObject->next();

			if (nextobj and nextobj->type() == OBJ_Empty)
				document()->remove(nextobj);

			// If the overlay object was there and the overlay itself is
			// not, remove the object.
			document()->remove(overlayObject);
			overlayObject = nullptr;
		}
		else if (meta.image and overlayObject == nullptr)
		{
			// Inverse case: image is there but the overlay object is
			// not, thus create the object.
			//
			// Find a suitable position to place this object. We want to place
			// this into the header, which is everything up to the first scemantic
			// object. If we find another overlay object, place this object after
			// the last one found. Otherwise, place it before the first schemantic
			// object and put an empty object after it (though don't do this if
			// there was no schemantic elements at all)
			int i;
			int lastOverlayPosition = -1;
			bool found = false;

			for (i = 0; i < document()->size(); ++i)
			{
				LDObject* object = document()->getObject (i);

				if (object->isScemantic())
				{
					found = true;
					break;
				}

				if (object->type() == OBJ_Overlay)
					lastOverlayPosition = i;
			}

			if (lastOverlayPosition != -1)
			{
				overlayObject = document()->emplaceAt<LDOverlay>(lastOverlayPosition + 1);
			}
			else
			{
				overlayObject = document()->emplaceAt<LDOverlay>(i);

				if (found)
					document()->emplaceAt<LDEmpty>(i + 1);
			}
		}

		if (meta.image and overlayObject)
		{
			overlayObject->setCamera (camera);
			overlayObject->setFileName (meta.fileName);
			overlayObject->setX (meta.offsetX);
			overlayObject->setY (meta.offsetY);
			overlayObject->setWidth (meta.width);
			overlayObject->setHeight (meta.height);
		}
	}

	if (m_window->renderer() == this)
		m_window->refresh();
}

// =============================================================================
//
void GLRenderer::highlightCursorObject()
{
	if (not m_config->highlightObjectBelowCursor() and objectAtCursor() == nullptr)
		return;

	LDObject* newObject = nullptr;
	LDObject* oldObject = objectAtCursor();
	qint32 newIndex;

	if (m_isCameraMoving or not m_config->highlightObjectBelowCursor())
	{
		newIndex = 0;
	}
	else
	{
		setPicking (true);
		drawGLScene();
		setPicking (false);

		unsigned char pixel[4];
		glReadPixels (m_mousePosition.x(), m_height - m_mousePosition.y(), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel[0]);
		newIndex = pixel[0] * 0x10000 | pixel[1] * 0x100 | pixel[2];
	}

	if (newIndex != (oldObject ? oldObject->id() : 0))
	{
		if (newIndex != 0)
			newObject = LDObject::fromID (newIndex);

		m_objectAtCursor = newObject;

		if (oldObject)
			compileObject (oldObject);

		if (newObject)
			compileObject (newObject);
	}

	update();
}

void GLRenderer::dragEnterEvent (QDragEnterEvent* ev)
{
	if (m_window and ev->source() == m_window->getPrimitivesTree() and m_window->getPrimitivesTree()->currentItem())
		ev->acceptProposedAction();
}

void GLRenderer::dropEvent (QDropEvent* ev)
{
	if (m_window and ev->source() == m_window->getPrimitivesTree())
	{
		PrimitiveTreeItem* item = static_cast<PrimitiveTreeItem*> (m_window->getPrimitivesTree()->currentItem());
		QString primitiveName = item->primitive()->name;
		LDSubfileReference* ref = currentDocument()->emplaceAt<LDSubfileReference>(m_window->suggestInsertPoint());
		ref->setFileInfo (m_documents->getDocumentByName (primitiveName));
		currentDocument()->addToSelection(ref);
		m_window->buildObjectList();
		m_window->renderer()->refresh();
		ev->acceptProposedAction();
	}
}

Vertex const& GLRenderer::position3D() const
{
	return m_position3D;
}

const CameraInfo& GLRenderer::cameraInfo (Camera camera) const
{
	if (valueInEnum<Camera>(camera))
		return g_cameraInfo[camera];
	else
		return g_cameraInfo[0];
}

bool GLRenderer::mouseHasMoved() const
{
	return m_totalMouseMove >= 10;
}

QPoint const& GLRenderer::mousePosition() const
{
	return m_mousePosition;
}

QPointF const& GLRenderer::mousePositionF() const
{
	return m_mousePositionF;
}

void GLRenderer::makeCurrent()
{
	QGLWidget::makeCurrent();
	initializeOpenGLFunctions();
}

int GLRenderer::depthNegateFactor() const
{
	return g_cameraInfo[camera()].negatedDepth ? -1 : 1;
}

Qt::KeyboardModifiers GLRenderer::keyboardModifiers() const
{
	return m_currentKeyboardModifiers;
}

Camera GLRenderer::camera() const
{
	return m_camera;
}

LDGLData& GLRenderer::currentDocumentData() const
{
	return *document()->glData();
}

double& GLRenderer::panning (Axis ax)
{
	return (ax == X) ? currentDocumentData().panX[camera()] :
		currentDocumentData().panY[camera()];
}

double GLRenderer::panning (Axis ax) const
{
	return (ax == X) ? currentDocumentData().panX[camera()] :
		currentDocumentData().panY[camera()];
}

double& GLRenderer::zoom()
{
	return currentDocumentData().zoom[camera()];
}


//
// ---------------------------------------------------------------------------------------------------------------------
//


LDGLOverlay::LDGLOverlay() :
	image(nullptr) {}

LDGLOverlay::~LDGLOverlay()
{
	delete image;
}

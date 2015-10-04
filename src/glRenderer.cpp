/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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
#include <QGLWidget>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QToolTip>
#include <QTextDocument>
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
#include "addObjectDialog.h"
#include "messageLog.h"
#include "glCompiler.h"
#include "primitives.h"
#include "documentmanager.h"

const LDFixedCamera g_FixedCameras[6] =
{
	{{  1,  0, 0 }, X, Z, false, false, false }, // top
	{{  0,  0, 0 }, X, Y, false,  true, false }, // front
	{{  0,  1, 0 }, Z, Y,  true,  true, false }, // left
	{{ -1,  0, 0 }, X, Z, false,  true, true }, // bottom
	{{  0,  0, 0 }, X, Y,  true,  true, true }, // back
	{{  0, -1, 0 }, Z, Y, false,  true, true }, // right
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
GLRenderer::GLRenderer (QWidget* parent) :
	QGLWidget (parent),
	HierarchyElement (parent),
	m_document (nullptr),
	m_initialized (false)
{
	m_isPicking = false;
	m_camera = (ECamera) m_config->camera();
	m_drawToolTip = false;
	m_currentEditMode = AbstractEditMode::createByType (this, EditModeType::Select);
	m_panning = false;
	m_compiler = new GLCompiler (this);
	m_objectAtCursor = nullptr;
	setDrawOnly (false);
	m_messageLog = new MessageManager (this);
	m_messageLog->setRenderer (this);
	m_width = m_height = -1;
	m_position3D = Origin;
	m_toolTipTimer = new QTimer (this);
	m_toolTipTimer->setSingleShot (true);
	m_isCameraMoving = false;
	m_thinBorderPen = QPen (QColor (0, 0, 0, 208), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	m_thinBorderPen.setWidth (1);
	setAcceptDrops (true);
	connect (m_toolTipTimer, SIGNAL (timeout()), this, SLOT (slot_toolTipTimer()));

	// Init camera icons
	for (ECamera cam = EFirstCamera; cam < ENumCameras; ++cam)
	{
		const char* cameraIconNames[ENumCameras] =
		{
			"camera-top", "camera-front", "camera-left",
			"camera-bottom", "camera-back", "camera-right",
			"camera-free"
		};

		CameraIcon* info = &m_cameraIcons[cam];
		info->image = new QPixmap (GetIcon (cameraIconNames[cam]));
		info->cam = cam;
	}

	calcCameraIcons();
}

// =============================================================================
//
GLRenderer::~GLRenderer()
{
	for (int i = 0; i < countof (currentDocumentData().overlays); ++i)
		delete currentDocumentData().overlays[i].img;

	for (CameraIcon& info : m_cameraIcons)
		delete info.image;

	if (messageLog())
		messageLog()->setRenderer (nullptr);

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
		int x1 = (m_width - (info.cam != EFreeCamera ? 48 : 16)) + ((i % 3) * 16) - 1;
		int y1 = ((i / 3) * 16) + 1;

		info.sourceRect = QRect (0, 0, 16, 16);
		info.targetRect = QRect (x1, y1, 16, 16);
		info.selRect = QRect (
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

MessageManager* GLRenderer::messageLog() const
{
	return m_messageLog;
}

bool GLRenderer::isPicking() const
{
	return m_isPicking;
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
	rotation (X) = 30.0f;
	rotation (Y) = 325.f;
	panning (X) = panning (Y) = rotation (Z) = 0.0f;
	needZoomToFit();
}

// =============================================================================
//
void GLRenderer::resetAllAngles()
{
	ECamera oldcam = camera();

	for (int i = 0; i < 7; ++i)
	{
		setCamera ((ECamera) i);
		resetAngles();
	}

	setCamera (oldcam);
}

// =============================================================================
//
void GLRenderer::initializeGL()
{
#ifdef USE_QT5
	initializeOpenGLFunctions();
#endif
	setBackground();
	glLineWidth (m_config->lineThickness());
	glLineStipple (1, 0x6666);
	setAutoFillBackground (false);
	setMouseTracking (true);
	setFocusPolicy (Qt::WheelFocus);
	compiler()->initialize();
	initializeAxes();
	m_initialized = true;
}

// =============================================================================
//
void GLRenderer::initializeAxes()
{
	// Definitions for visual axes, drawn on the screen
	struct AxisInfo
	{
		QColor color;
		Vertex extrema;
	};

	static const AxisInfo axisinfo[3] =
	{
		{ QColor (192,  96,  96), Vertex (10000, 0, 0) }, // X
		{ QColor (48,  192,  48), Vertex (0, 10000, 0) }, // Y
		{ QColor (48,  112, 192), Vertex (0, 0, 10000) }, // Z
	};

	float axisdata[18];
	float colordata[18];
	memset (axisdata, 0, sizeof axisdata);

	for (int i = 0; i < 3; ++i)
	{
		const AxisInfo& data = axisinfo[i];

		for_axes (ax)
		{
			axisdata[(i * 6) + ax] = data.extrema[ax];
			axisdata[(i * 6) + 3 + ax] = -data.extrema[ax];
		}

		int offset = i * 6;
		colordata[offset + 0] = colordata[offset + 3] = data.color.red();
		colordata[offset + 1] = colordata[offset + 4] = data.color.green();
		colordata[offset + 2] = colordata[offset + 5] = data.color.blue();
	}

	glGenBuffers (1, &m_axesVbo);
	glBindBuffer (GL_ARRAY_BUFFER, m_axesVbo);
	glBufferData (GL_ARRAY_BUFFER, sizeof axisdata, axisdata, GL_STATIC_DRAW);
	glGenBuffers (1, &m_axesColorVbo);
	glBindBuffer (GL_ARRAY_BUFFER, m_axesColorVbo);
	glBufferData (GL_ARRAY_BUFFER, sizeof colordata, colordata, GL_STATIC_DRAW);
	glBindBuffer (GL_ARRAY_BUFFER, 0);
}

// =============================================================================
//
void GLRenderer::setBackground()
{
	if (isPicking())
	{
		glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
		return;
	}

	QColor color = m_config->backgroundColor();

	if (not color.isValid())
		return;

	color.setAlpha (255);
	m_useDarkBackground = luma (color) < 80;
	m_backgroundColor = color;
	qglClearColor (color);
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

	if (m_config->drawWireframe() and not isPicking())
		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable (GL_DEPTH_TEST);

	if (camera() != EFreeCamera)
	{
		glMatrixMode (GL_PROJECTION);
		glPushMatrix();

		glLoadIdentity();
		glOrtho (-m_virtualWidth, m_virtualWidth, -m_virtualHeight, m_virtualHeight, -100.0f, 100.0f);
		glTranslatef (panning (X), panning (Y), 0.0f);

		if (camera() != EFrontCamera and camera() != EBackCamera)
		{
			glRotatef (90.0f, g_FixedCameras[camera()].glrotate[0],
				g_FixedCameras[camera()].glrotate[1],
				g_FixedCameras[camera()].glrotate[2]);
		}

		// Back camera needs to be handled differently
		if (camera() == EBackCamera)
		{
			glRotatef (180.0f, 1.0f, 0.0f, 0.0f);
			glRotatef (180.0f, 0.0f, 0.0f, 1.0f);
		}
	}
	else
	{
		glMatrixMode (GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glTranslatef (0.0f, 0.0f, -2.0f);
		glTranslatef (panning (X), panning (Y), -zoom());
		glRotatef (rotation (X), 1.0f, 0.0f, 0.0f);
		glRotatef (rotation (Y), 0.0f, 1.0f, 0.0f);
		glRotatef (rotation (Z), 0.0f, 0.0f, 1.0f);
	}

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);

	if (isPicking())
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

	int surfacenum = m_compiler->vboNumber (surface, SurfacesVboComplement);
	int colornum = m_compiler->vboNumber (surface, colors);
	m_compiler->prepareVBO (surfacenum);
	m_compiler->prepareVBO (colornum);
	GLuint surfacevbo = m_compiler->vbo (surfacenum);
	GLuint colorvbo = m_compiler->vbo (colornum);
	GLsizei count = m_compiler->vboSize (surfacenum) / 3;

	if (count > 0)
	{
		glBindBuffer (GL_ARRAY_BUFFER, surfacevbo);
		glVertexPointer (3, GL_FLOAT, 0, nullptr);
		CHECK_GL_ERROR();
		glBindBuffer (GL_ARRAY_BUFFER, colorvbo);
		glColorPointer (4, GL_FLOAT, 0, nullptr);
		CHECK_GL_ERROR();
		glDrawArrays (type, 0, count);
		CHECK_GL_ERROR();
	}
}

// =============================================================================
//
// This converts a 2D point on the screen to a 3D point in the model. If 'snap'
// is true, the 3D point will snap to the current grid.
//
Vertex GLRenderer::convert2dTo3d (const QPoint& pos2d, bool snap) const
{
	if (camera() == EFreeCamera)
		return Origin;

	Vertex pos3d;
	const LDFixedCamera* cam = &g_FixedCameras[camera()];
	const Axis axisX = cam->localX;
	const Axis axisY = cam->localY;
	const int negXFac = cam->negatedX ? -1 : 1,
				negYFac = cam->negatedY ? -1 : 1;

	// Calculate cx and cy - these are the LDraw unit coords the cursor is at.
	double cx = (-m_virtualWidth + ((2 * pos2d.x() * m_virtualWidth) / m_width) - panning (X));
	double cy = (m_virtualHeight - ((2 * pos2d.y() * m_virtualHeight) / m_height) - panning (Y));

	if (snap)
	{
		cx = Grid::Snap (cx, Grid::Coordinate);
		cy = Grid::Snap (cy, Grid::Coordinate);
	}

	cx *= negXFac;
	cy *= negYFac;

	RoundToDecimals (cx, 4);
	RoundToDecimals (cy, 4);

	// Create the vertex from the coordinates
	pos3d.setCoordinate (axisX, cx);
	pos3d.setCoordinate (axisY, cy);
	pos3d.setCoordinate ((Axis) (3 - axisX - axisY), getDepthValue());
	return pos3d;
}

// =============================================================================
//
// Inverse operation for the above - convert a 3D position to a 2D screen position. Don't ask me how this code manages
// to work, I don't even know.
//
QPoint GLRenderer::convert3dTo2d (const Vertex& pos3d)
{
	if (camera() == EFreeCamera)
		return QPoint (0, 0);

	const LDFixedCamera* cam = &g_FixedCameras[camera()];
	const Axis axisX = cam->localX;
	const Axis axisY = cam->localY;
	const int negXFac = cam->negatedX ? -1 : 1;
	const int negYFac = cam->negatedY ? -1 : 1;
	GLfloat matrix[16];
	const double x = pos3d.x();
	const double y = pos3d.y();
	const double z = pos3d.z();
	Vertex transformed;

	glGetFloatv (GL_MODELVIEW_MATRIX, matrix);
	transformed.setX ((matrix[0] * x) + (matrix[1] * y) + (matrix[2] * z) + matrix[3]);
	transformed.setY ((matrix[4] * x) + (matrix[5] * y) + (matrix[6] * z) + matrix[7]);
	transformed.setZ ((matrix[8] * x) + (matrix[9] * y) + (matrix[10] * z) + matrix[11]);
	double rx = (((transformed[axisX] * negXFac) + m_virtualWidth + panning (X)) * m_width) / (2 * m_virtualWidth);
	double ry = (((transformed[axisY] * negYFac) - m_virtualHeight + panning (Y)) * m_height) / (2 * m_virtualHeight);
	return QPoint (rx, -ry);
}

QPen GLRenderer::textPen() const
{
	return QPen (m_useDarkBackground ? Qt::white : Qt::black);
}

QPen GLRenderer::linePen() const
{
	QPen linepen (m_thinBorderPen);
	linepen.setWidth (2);
	linepen.setColor (luma (m_backgroundColor) < 40 ? Qt::white : Qt::black);
	return linepen;
}

// =============================================================================
//
void GLRenderer::paintEvent (QPaintEvent*)
{
	doMakeCurrent();
	m_virtualWidth = zoom();
	m_virtualHeight = (m_height * m_virtualWidth) / m_width;
	initGLData();
	drawGLScene();

	QPainter paint (this);
	QFontMetrics metrics = QFontMetrics (QFont());
	paint.setRenderHint (QPainter::HighQualityAntialiasing);

	// If we wish to only draw the brick, stop here
	if (isDrawOnly())
		return;

#ifndef RELEASE
	if (not isPicking())
	{
		QString text = format ("Rotation: (%1°, %2°, %3°)\nPanning: (%4, %5), Zoom: %6",
			rotation(X), rotation(Y), rotation(Z), panning(X), panning(Y), zoom());
		QRect textSize = metrics.boundingRect (0, 0, m_width, m_height, Qt::AlignCenter, text);
		paint.setPen (textPen());
		paint.drawText ((width() - textSize.width()) / 2, height() - textSize.height(), textSize.width(),
			textSize.height(), Qt::AlignCenter, text);
	}
#endif

	if (camera() != EFreeCamera and not isPicking())
	{
		// Paint the overlay image if we have one
		const LDGLOverlay& overlay = currentDocumentData().overlays[camera()];

		if (overlay.img)
		{
			QPoint v0 = convert3dTo2d (currentDocumentData().overlays[camera()].v0);
			QPoint v1 = convert3dTo2d (currentDocumentData().overlays[camera()].v1);
			QRect targetRect (v0.x(), v0.y(), qAbs (v1.x() - v0.x()), qAbs (v1.y() - v0.y()));
			QRect sourceRect (0, 0, overlay.img->width(), overlay.img->height());
			paint.drawImage (targetRect, *overlay.img, sourceRect);
		}

		// Paint the coordinates onto the screen.
		QString text = format (tr ("X: %1, Y: %2, Z: %3"), m_position3D[X], m_position3D[Y], m_position3D[Z]);
		QFontMetrics metrics = QFontMetrics (font());
		QRect textSize = metrics.boundingRect (0, 0, m_width, m_height, Qt::AlignCenter, text);
		paint.setPen (textPen());
		paint.drawText (m_width - textSize.width(), m_height - 16, textSize.width(),
			textSize.height(), Qt::AlignCenter, text);
	}

	if (not isPicking())
	{
		// Draw edit mode HUD
		m_currentEditMode->render (paint);

		// Draw a background for the selected camera
		paint.setPen (m_thinBorderPen);
		paint.setBrush (QBrush (QColor (0, 128, 160, 128)));
		paint.drawRect (m_cameraIcons[camera()].selRect);

		// Draw the camera icons
		for (CameraIcon& info : m_cameraIcons)
		{
			// Don't draw the free camera icon when we can't use the free camera
			if (&info == &m_cameraIcons[EFreeCamera] and not m_currentEditMode->allowFreeCamera())
				continue;

			paint.drawPixmap (info.targetRect, *info.image, info.sourceRect);
		}

		// Draw a label for the current camera in the bottom left corner
		{
			const int margin = 4;
			paint.setPen (textPen());
			paint.drawText (QPoint (margin, height() - (margin + metrics.descent())), currentCameraName());
		}

		// Tool tips
		if (m_drawToolTip)
		{
			if (not m_cameraIcons[m_toolTipCamera].targetRect.contains (m_mousePosition))
				m_drawToolTip = false;
			else
				QToolTip::showText (m_globalpos, currentCameraName());
		}
	}

	// Message log
	if (messageLog())
	{
		int y = 0;
		const int margin = 2;
		QColor penColor = textPen().color();

		for (const MessageManager::Line& line : messageLog()->getLines())
		{
			penColor.setAlphaF (line.alpha);
			paint.setPen (penColor);
			paint.drawText (QPoint (margin, y + margin + metrics.ascent()), line.text);
			y += metrics.height();
		}
	}
}

// =============================================================================
//
void GLRenderer::drawBlip (QPainter& painter, QPointF pos, QColor color) const
{
	QPen pen = m_thinBorderPen;
	const int blipsize = 8;
	pen.setWidth (1);
	painter.setPen (pen);
	painter.setBrush (color);
	painter.drawEllipse (pos.x() - blipsize / 2, pos.y() - blipsize / 2, blipsize, blipsize);
}

void GLRenderer::drawBlipCoordinates (QPainter& painter, const Vertex& pos3d)
{
	drawBlipCoordinates (painter, pos3d, convert3dTo2d (pos3d));
}

void GLRenderer::drawBlipCoordinates (QPainter& painter, const Vertex& pos3d, QPointF pos)
{
	painter.setPen (textPen());
	painter.drawText (pos.x(), pos.y() - 8, pos3d.toString (true));
}

// =============================================================================
//
void GLRenderer::clampAngle (double& angle) const
{
	while (angle < 0)
		angle += 360.0;

	while (angle > 360.0)
		angle -= 360.0;
}

// =============================================================================
//
void GLRenderer::mouseReleaseEvent (QMouseEvent* ev)
{
	bool wasLeft = (m_lastButtons & Qt::LeftButton) and not (ev->buttons() & Qt::LeftButton);
	Qt::MouseButtons releasedbuttons = m_lastButtons & ~ev->buttons();
	m_panning = false;

	if (wasLeft)
	{
		// Check if we selected a camera icon
		if (not mouseHasMoved())
		{
			for (CameraIcon & info : m_cameraIcons)
			{
				if (info.targetRect.contains (ev->pos()))
				{
					setCamera (info.cam);
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
void GLRenderer::mousePressEvent (QMouseEvent* ev)
{
	m_totalMouseMove = 0;
	m_lastButtons = ev->buttons();

	if (m_currentEditMode->mousePressed (ev))
		ev->accept();
}

// =============================================================================
//
void GLRenderer::mouseMoveEvent (QMouseEvent* ev)
{
	int dx = ev->x() - m_mousePosition.x();
	int dy = ev->y() - m_mousePosition.y();
	m_totalMouseMove += qAbs (dx) + qAbs (dy);
	m_isCameraMoving = false;

	if (not m_currentEditMode->mouseMoved (ev))
	{
		const bool left = ev->buttons() & Qt::LeftButton,
				mid = ev->buttons() & Qt::MidButton,
				shift = ev->modifiers() & Qt::ShiftModifier;

		if (mid or (left and shift))
		{
			panning (X) += 0.03f * dx * (zoom() / 7.5f);
			panning (Y) -= 0.03f * dy * (zoom() / 7.5f);
			m_panning = true;
			m_isCameraMoving = true;
		}
		else if (left and camera() == EFreeCamera)
		{
			rotation (X) = rotation (X) + dy;
			rotation (Y) = rotation (Y) + dx;
			clampAngle (rotation (X));
			clampAngle (rotation (Y));
			m_isCameraMoving = true;
		}
	}

	// Start the tool tip timer
	if (not m_drawToolTip)
		m_toolTipTimer->start (500);

	// Update 2d position
	m_mousePosition = ev->pos();
	m_globalpos = ev->globalPos();

#ifndef USE_QT5
	m_mousePositionF = ev->posF();
#else
	m_mousePositionF = ev->localPos();
#endif

	// Calculate 3d position of the cursor
	m_position3D = (camera() != EFreeCamera) ? convert2dTo3d (m_mousePosition, true) : Origin;

	highlightCursorObject();
	update();
	ev->accept();
}

// =============================================================================
//
void GLRenderer::keyPressEvent (QKeyEvent* ev)
{
	m_currentKeyboardModifiers = ev->modifiers();
}

// =============================================================================
//
void GLRenderer::keyReleaseEvent (QKeyEvent* ev)
{
	m_currentKeyboardModifiers = ev->modifiers();
	m_currentEditMode->keyReleased (ev);
	update();
}

// =============================================================================
//
void GLRenderer::wheelEvent (QWheelEvent* ev)
{
	doMakeCurrent();

	zoomNotch (ev->delta() > 0);
	zoom() = qBound (0.01, zoom(), 10000.0);
	m_isCameraMoving = true;
	update();
	ev->accept();
}

// =============================================================================
//
void GLRenderer::leaveEvent (QEvent* ev)
{
	(void) ev;
	m_drawToolTip = false;
	m_toolTipTimer->stop();
	update();
}

// =============================================================================
//
void GLRenderer::contextMenuEvent (QContextMenuEvent* ev)
{
	m_window->spawnContextMenu (ev->globalPos());
}

// =============================================================================
//
void GLRenderer::setCamera (const ECamera cam)
{
	// The edit mode may forbid the free camera.
	if (cam == EFreeCamera and not m_currentEditMode->allowFreeCamera())
		return;

	m_camera = cam;
	m_config->setCamera ((int) cam);
	m_window->updateEditModeActions();
}

// =============================================================================
//
void GLRenderer::pick (int mouseX, int mouseY, bool additive)
{
	pick (QRect (mouseX, mouseY, mouseX + 1, mouseY + 1), additive);
}

// =============================================================================
//
void GLRenderer::pick (QRect const& range, bool additive)
{
	doMakeCurrent();

	// Clear the selection if we do not wish to add to it.
	if (not additive)
	{
		LDObjectList oldSelection = selectedObjects();
		currentDocument()->clearSelection();

		for (LDObject* obj : oldSelection)
			compileObject (obj);
	}

	// Paint the picking scene
	setPicking (true);
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
	uchar* const pixeldata = new uchar[4 * numpixels];
	uchar* pixelptr = &pixeldata[0];

	// Read pixels from the color buffer.
	glReadPixels (x0, m_height - y1, areawidth, areaheight, GL_RGBA, GL_UNSIGNED_BYTE, pixeldata);

	LDObject* removedObj = nullptr;
	QList<qint32> indices;

	// Go through each pixel read and add them to the selection.
	// Note: black is background, those indices are skipped.
	for (qint32 i = 0; i < numpixels; ++i)
	{
		qint32 idx =
			(*(pixelptr + 0) * 0x10000) +
			(*(pixelptr + 1) * 0x100) +
			*(pixelptr + 2);
		pixelptr += 4;

		if (idx != 0)
			indices << idx;
	}

	removeDuplicates (indices);

	for (qint32 idx : indices)
	{
		LDObject* obj = LDObject::fromID (idx);

		if (obj == nullptr)
			continue;

		// If this is an additive single pick and the object is currently selected,
		// we remove it from selection instead.
		if (additive)
		{
			if (obj->isSelected())
			{
				obj->deselect();
				removedObj = obj;
				break;
			}
		}

		obj->select();
	}

	delete[] pixeldata;

	// Update everything now.
	m_window->updateSelection();

	// Recompile the objects now to update their color
	for (LDObject* obj : selectedObjects())
		compileObject (obj);

	if (removedObj)
		compileObject (removedObj);

	setPicking (false);
	repaint();
}

//
// Simpler version of GLRenderer::pick which simply picks whatever object on the screen
//
LDObject* GLRenderer::pickOneObject (int mouseX, int mouseY)
{
	uchar pixel[4];
	doMakeCurrent();
	setPicking (true);
	drawGLScene();
	glReadPixels (mouseX, m_height - mouseY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	LDObject* obj = LDObject::fromID ((pixel[0] * 0x10000) + (pixel[1] * 0x100) + pixel[2]);
	setPicking (false);
	repaint();
	return obj;
}

// =============================================================================
//
void GLRenderer::setEditMode (EditModeType a)
{
	if (m_currentEditMode and m_currentEditMode->type() == a)
		return;

	delete m_currentEditMode;
	m_currentEditMode = AbstractEditMode::createByType (this, a);

	// If we cannot use the free camera, use the top one instead.
	if (camera() == EFreeCamera and not m_currentEditMode->allowFreeCamera())
		setCamera (ETopCamera);

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
void GLRenderer::setDocument (LDDocument* document)
{
	m_document = document;

	if (document)
	{
		initOverlaysFromObjects();

		if (not currentDocumentData().init)
		{
			resetAllAngles();
			currentDocumentData().init = true;
		}

		currentDocumentData().needZoomToFit = true;
	}
}

// =============================================================================
//
void GLRenderer::setPicking (bool value)
{
	m_isPicking = value;
	setBackground();

	if (isPicking())
	{
		glDisable (GL_DITHER);

		// Use particularly thick lines while picking ease up selecting lines.
		glLineWidth (qMax<double> (m_config->lineThickness(), 6.5));
	}
	else
	{
		glEnable (GL_DITHER);

		// Restore line thickness
		glLineWidth (m_config->lineThickness());
	}
}

// =============================================================================
//
void GLRenderer::getRelativeAxes (Axis& relX, Axis& relY) const
{
	const LDFixedCamera* cam = &g_FixedCameras[camera()];
	relX = cam->localX;
	relY = cam->localY;
}

// =============================================================================
//
Axis GLRenderer::getRelativeZ() const
{
	const LDFixedCamera* cam = &g_FixedCameras[camera()];
	return (Axis) (3 - cam->localX - cam->localY);
}

// =============================================================================
//
void GLRenderer::compileObject (LDObject* obj)
{
	compiler()->stageForCompilation (obj);
}

// =============================================================================
//
void GLRenderer::forgetObject (LDObject* obj)
{
	compiler()->dropObjectInfo (obj);
	compiler()->unstage (obj);

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
	glReadPixels (0, 0, width(), height(), GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<uchar*> (result.data()));
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
			m_toolTipCamera = icon.cam;
			m_drawToolTip = true;
			update();
			break;
		}
	}
}

// =============================================================================
//
Axis GLRenderer::getCameraAxis (bool y, ECamera camid)
{
	if (camid == (ECamera) -1)
		camid = camera();

	const LDFixedCamera* cam = &g_FixedCameras[camid];
	return (y) ? cam->localY : cam->localX;
}

// =============================================================================
//
bool GLRenderer::setupOverlay (ECamera cam, QString file, int x, int y, int w, int h)
{
	QImage* img = new QImage (QImage (file).convertToFormat (QImage::Format_ARGB32));
	LDGLOverlay& info = getOverlay (cam);

	if (img->isNull())
	{
		Critical (tr ("Failed to load overlay image!"));
		currentDocumentData().overlays[cam].invalid = true;
		delete img;
		return false;
	}

	delete info.img; // delete the old image

	info.fname = file;
	info.lw = w;
	info.lh = h;
	info.ox = x;
	info.oy = y;
	info.img = img;
	info.invalid = false;

	if (info.lw == 0)
		info.lw = (info.lh * img->width()) / img->height();
	else if (info.lh == 0)
		info.lh = (info.lw * img->height()) / img->width();

	const Axis x2d = getCameraAxis (false, cam),
		y2d = getCameraAxis (true, cam);
	const double negXFac = g_FixedCameras[cam].negatedX ? -1 : 1,
		negYFac = g_FixedCameras[cam].negatedY ? -1 : 1;

	info.v0 = info.v1 = Origin;
	info.v0.setCoordinate (x2d, -(info.ox * info.lw * negXFac) / img->width());
	info.v0.setCoordinate (y2d, (info.oy * info.lh * negYFac) / img->height());
	info.v1.setCoordinate (x2d, info.v0[x2d] + info.lw);
	info.v1.setCoordinate (y2d, info.v0[y2d] + info.lh);

	// Set alpha of all pixels to 0.5
	for (long i = 0; i < img->width(); ++i)
	for (long j = 0; j < img->height(); ++j)
	{
		uint32 pixel = img->pixel (i, j);
		img->setPixel (i, j, 0x80000000 | (pixel & 0x00FFFFFF));
	}

	updateOverlayObjects();
	return true;
}

// =============================================================================
//
void GLRenderer::clearOverlay()
{
	if (camera() == EFreeCamera)
		return;

	LDGLOverlay& info = currentDocumentData().overlays[camera()];
	delete info.img;
	info.img = nullptr;

	updateOverlayObjects();
}

// =============================================================================
//
void GLRenderer::setDepthValue (double depth)
{
	if (camera() < EFreeCamera)
		currentDocumentData().depthValues[camera()] = depth;
}

// =============================================================================
//
double GLRenderer::getDepthValue() const
{
	if (camera() < EFreeCamera)
		return currentDocumentData().depthValues[camera()];
	else
		return 0.0;
}

// =============================================================================
//
QString GLRenderer::cameraName (ECamera camera) const
{
	switch (camera)
	{
	case ETopCamera: return tr ("Top Camera");
	case EFrontCamera: return tr ("Front Camera");
	case ELeftCamera: return tr ("Left Camera");
	case EBottomCamera: return tr ("Bottom Camera");
	case EBackCamera: return tr ("Back Camera");
	case ERightCamera: return tr ("Right Camera");
	case EFreeCamera: return tr ("Free Camera");
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
LDOverlay* GLRenderer::findOverlayObject (ECamera cam)
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
	for (ECamera cam = EFirstCamera; cam < ENumCameras; ++cam)
	{
		if (cam == EFreeCamera)
			continue;

		LDGLOverlay& meta = currentDocumentData().overlays[cam];
		LDOverlay* ovlobj = findOverlayObject (cam);

		if (ovlobj == nullptr and meta.img)
		{
			delete meta.img;
			meta.img = nullptr;
		}
		else if (ovlobj and
			(meta.img == nullptr or meta.fname != ovlobj->fileName()) and
			not meta.invalid)
		{
			setupOverlay (cam, ovlobj->fileName(), ovlobj->x(),
				ovlobj->y(), ovlobj->width(), ovlobj->height());
		}
	}
}

// =============================================================================
//
void GLRenderer::updateOverlayObjects()
{
	for (ECamera cam = EFirstCamera; cam < ENumCameras; ++cam)
	{
		if (cam == EFreeCamera)
			continue;

		LDGLOverlay& meta = currentDocumentData().overlays[cam];
		LDOverlay* ovlobj = findOverlayObject (cam);

		if (meta.img == nullptr and ovlobj)
		{
			// If this is the last overlay image, we need to remove the empty space after it as well.
			LDObject* nextobj = ovlobj->next();

			if (nextobj and nextobj->type() == OBJ_Empty)
				nextobj->destroy();

			// If the overlay object was there and the overlay itself is
			// not, remove the object.
			ovlobj->destroy();
		}
		else if (meta.img and ovlobj == nullptr)
		{
			// Inverse case: image is there but the overlay object is
			// not, thus create the object.
			ovlobj = LDSpawn<LDOverlay>();

			// Find a suitable position to place this object. We want to place
			// this into the header, which is everything up to the first scemantic
			// object. If we find another overlay object, place this object after
			// the last one found. Otherwise, place it before the first schemantic
			// object and put an empty object after it (though don't do this if
			// there was no schemantic elements at all)
			int i, lastOverlay = -1;
			bool found = false;

			for (i = 0; i < document()->getObjectCount(); ++i)
			{
				LDObject* obj = document()->getObject (i);

				if (obj->isScemantic())
				{
					found = true;
					break;
				}

				if (obj->type() == OBJ_Overlay)
					lastOverlay = i;
			}

			if (lastOverlay != -1)
				document()->insertObj (lastOverlay + 1, ovlobj);
			else
			{
				document()->insertObj (i, ovlobj);

				if (found)
					document()->insertObj (i + 1, LDSpawn<LDEmpty>());
			}
		}

		if (meta.img and ovlobj)
		{
			ovlobj->setCamera (cam);
			ovlobj->setFileName (meta.fname);
			ovlobj->setX (meta.ox);
			ovlobj->setY (meta.oy);
			ovlobj->setWidth (meta.lw);
			ovlobj->setHeight (meta.lh);
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
		LDSubfile* ref = LDSpawn<LDSubfile>();
		ref->setColor (MainColor);
		ref->setFileInfo (m_documents->getDocumentByName (primitiveName));
		ref->setPosition (Origin);
		ref->setTransform (IdentityMatrix);
		currentDocument()->insertObj (m_window->suggestInsertPoint(), ref);
		ref->select();
		m_window->buildObjectList();
		m_window->renderer()->refresh();
		ev->acceptProposedAction();
	}
}

Vertex const& GLRenderer::position3D() const
{
	return m_position3D;
}

const LDFixedCamera& GLRenderer::getFixedCamera (ECamera cam) const
{
	return g_FixedCameras[cam];
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

void GLRenderer::doMakeCurrent()
{
	makeCurrent();
	initializeOpenGLFunctions();
}

int GLRenderer::depthNegateFactor() const
{
	return g_FixedCameras[camera()].negatedDepth ? -1 : 1;
}

Qt::KeyboardModifiers GLRenderer::keyboardModifiers() const
{
	return m_currentKeyboardModifiers;
}

ECamera GLRenderer::camera() const
{
	return m_camera;
}

LDGLData& GLRenderer::currentDocumentData() const
{
	return *document()->glData();
}

double& GLRenderer::rotation (Axis ax)
{
	return
		(ax == X) ? currentDocumentData().rotationX :
		(ax == Y) ? currentDocumentData().rotationY :
					currentDocumentData().rotationZ;
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

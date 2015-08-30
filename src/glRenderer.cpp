/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
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
#include "configuration.h"
#include "ldDocument.h"
#include "glRenderer.h"
#include "colors.h"
#include "mainWindow.h"
#include "miscallenous.h"
#include "editHistory.h"
#include "dialogs.h"
#include "addObjectDialog.h"
#include "messageLog.h"
#include "glCompiler.h"
#include "primitives.h"

const LDFixedCamera g_FixedCameras[6] =
{
	{{  1,  0, 0 }, X, Z, false, false, false }, // top
	{{  0,  0, 0 }, X, Y, false,  true, false }, // front
	{{  0,  1, 0 }, Z, Y,  true,  true, false }, // left
	{{ -1,  0, 0 }, X, Z, false,  true, true }, // bottom
	{{  0,  0, 0 }, X, Y,  true,  true, true }, // back
	{{  0, -1, 0 }, Z, Y, false,  true, true }, // right
};

CFGENTRY (String, BackgroundColor,			"#FFFFFF")
CFGENTRY (String, MainColor,					"#A0A0A0")
CFGENTRY (Float, MainColorAlpha,				1.0)
CFGENTRY (Int, LineThickness,				2)
CFGENTRY (Bool, BFCRedGreenView,			false)
CFGENTRY (Int, Camera,						EFreeCamera)
CFGENTRY (Bool, BlackEdges,					false)
CFGENTRY (Bool, DrawAxes,					false)
CFGENTRY (Bool, DrawWireframe,				false)
CFGENTRY (Bool, UseLogoStuds,				false)
CFGENTRY (Bool, AntiAliasedLines,			true)
CFGENTRY (Bool, RandomColors,				false)
CFGENTRY (Bool, HighlightObjectBelowCursor,	true)
CFGENTRY (Bool, DrawSurfaces,				true)
CFGENTRY (Bool, DrawEdgeLines,				true)
CFGENTRY (Bool, DrawConditionalLines,		true)

// argh
const char* g_CameraNames[7] =
{
	QT_TRANSLATE_NOOP ("GLRenderer",  "Top"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Front"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Left"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Bottom"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Back"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Right"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Free")
};

struct LDGLAxis
{
	const QColor col;
	const Vertex vert;
};

// Definitions for visual axes, drawn on the screen
static const LDGLAxis g_GLAxes[3] =
{
	{ QColor (192,  96,  96), Vertex (10000, 0, 0) }, // X
	{ QColor (48,  192,  48), Vertex (0, 10000, 0) }, // Y
	{ QColor (48,  112, 192), Vertex (0, 0, 10000) }, // Z
};

static bool RendererInitialized (false);

// =============================================================================
//
GLRenderer::GLRenderer (QWidget* parent) : QGLWidget (parent)
{
	m_isPicking = false;
	m_camera = (ECamera) cfg::Camera;
	m_drawToolTip = false;
	m_editmode = AbstractEditMode::createByType (this, EditModeType::Select);
	m_panning = false;
	m_compiler = new GLCompiler (this);
	m_objectAtCursor = nullptr;
	setDrawOnly (false);
	setMessageLog (null);
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
		QString iconname = format ("camera-%1", tr (g_CameraNames[cam]).toLower());
		CameraIcon* info = &m_cameraIcons[cam];
		info->img = new QPixmap (GetIcon (iconname));
		info->cam = cam;
	}

	calcCameraIcons();
}

// =============================================================================
//
GLRenderer::~GLRenderer()
{
	for (int i = 0; i < 6; ++i)
		delete currentDocumentData().overlays[i].img;

	for (CameraIcon& info : m_cameraIcons)
		delete info.img;

	if (messageLog())
		messageLog()->setRenderer (null);

	m_compiler->setRenderer (null);
	delete m_compiler;
	delete m_editmode;

	glDeleteBuffers (1, &m_axesVBO);
	glDeleteBuffers (1, &m_axesColorVBO);
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
		const long x1 = (m_width - (info.cam != EFreeCamera ? 48 : 16)) + ((i % 3) * 16) - 1,
			y1 = ((i / 3) * 16) + 1;

		info.srcRect = QRect (0, 0, 16, 16);
		info.destRect = QRect (x1, y1, 16, 16);
		info.selRect = QRect (
			info.destRect.x(),
			info.destRect.y(),
			info.destRect.width() + 1,
			info.destRect.height() + 1
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

	if (cfg::AntiAliasedLines)
	{
		glEnable (GL_LINE_SMOOTH);
		glEnable (GL_POLYGON_SMOOTH);
		glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint (GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	} else
	{
		glDisable (GL_LINE_SMOOTH);
		glDisable (GL_POLYGON_SMOOTH);
	}
}

// =============================================================================
//
void GLRenderer::needZoomToFit()
{
	if (document() != null)
		currentDocumentData().needZoomToFit = true;
}

// =============================================================================
//
void GLRenderer::resetAngles()
{
	rot (X) = 30.0f;
	rot (Y) = 325.f;
	pan (X) = pan (Y) = rot (Z) = 0.0f;
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
	glLineWidth (cfg::LineThickness);
	glLineStipple (1, 0x6666);
	setAutoFillBackground (false);
	setMouseTracking (true);
	setFocusPolicy (Qt::WheelFocus);
	compiler()->initialize();
	initializeAxes();
	RendererInitialized = true;
}

// =============================================================================
//
void GLRenderer::initializeAxes()
{
	float axesdata[18];
	float colordata[18];
	memset (axesdata, 0, sizeof axesdata);

	for (int i = 0; i < 3; ++i)
	{
		for_axes (ax)
		{
			axesdata[(i * 6) + ax] = g_GLAxes[i].vert[ax];
			axesdata[(i * 6) + 3 + ax] = -g_GLAxes[i].vert[ax];
		}

		for (int j = 0; j < 2; ++j)
		{
			colordata[(i * 6) + (j * 3) + 0] = g_GLAxes[i].col.red();
			colordata[(i * 6) + (j * 3) + 1] = g_GLAxes[i].col.green();
			colordata[(i * 6) + (j * 3) + 2] = g_GLAxes[i].col.blue();
		}
	}

	glGenBuffers (1, &m_axesVBO);
	glBindBuffer (GL_ARRAY_BUFFER, m_axesVBO);
	glBufferData (GL_ARRAY_BUFFER, sizeof axesdata, axesdata, GL_STATIC_DRAW);
	glGenBuffers (1, &m_axesColorVBO);
	glBindBuffer (GL_ARRAY_BUFFER, m_axesColorVBO);
	glBufferData (GL_ARRAY_BUFFER, sizeof colordata, colordata, GL_STATIC_DRAW);
	glBindBuffer (GL_ARRAY_BUFFER, 0);
}

// =============================================================================
//
QColor GLRenderer::getMainColor()
{
	QColor col (cfg::MainColor);

	if (not col.isValid())
		return QColor (0, 0, 0);

	col.setAlpha (cfg::MainColorAlpha * 255.f);
	return col;
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

	QColor col (cfg::BackgroundColor);

	if (not col.isValid())
		return;

	col.setAlpha (255);

	m_darkbg = Luma (col) < 80;
	m_bgcolor = col;
	qglClearColor (col);
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
	if (not RendererInitialized)
		return;

	compiler()->compileDocument (CurrentDocument());
	refresh();
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
	if (document() == null)
		return;

	if (currentDocumentData().needZoomToFit)
	{
		currentDocumentData().needZoomToFit = false;
		zoomAllToFit();
	}

	if (cfg::DrawWireframe and not isPicking())
		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable (GL_DEPTH_TEST);

	if (camera() != EFreeCamera)
	{
		glMatrixMode (GL_PROJECTION);
		glPushMatrix();

		glLoadIdentity();
		glOrtho (-m_virtWidth, m_virtWidth, -m_virtHeight, m_virtHeight, -100.0f, 100.0f);
		glTranslatef (pan (X), pan (Y), 0.0f);

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
		glTranslatef (pan (X), pan (Y), -zoom());
		glRotatef (rot (X), 1.0f, 0.0f, 0.0f);
		glRotatef (rot (Y), 0.0f, 1.0f, 0.0f);
		glRotatef (rot (Z), 0.0f, 0.0f, 1.0f);
	}

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);

	if (isPicking())
	{
		drawVBOs (VBOSF_Triangles, VBOCM_PickColors, GL_TRIANGLES);
		drawVBOs (VBOSF_Quads, VBOCM_PickColors, GL_QUADS);
		drawVBOs (VBOSF_Lines, VBOCM_PickColors, GL_LINES);
		drawVBOs (VBOSF_CondLines, VBOCM_PickColors, GL_LINES);
	}
	else
	{
		if (cfg::BFCRedGreenView)
		{
			glEnable (GL_CULL_FACE);
			glCullFace (GL_BACK);
			drawVBOs (VBOSF_Triangles, VBOCM_BFCFrontColors, GL_TRIANGLES);
			drawVBOs (VBOSF_Quads, VBOCM_BFCFrontColors, GL_QUADS);
			glCullFace (GL_FRONT);
			drawVBOs (VBOSF_Triangles, VBOCM_BFCBackColors, GL_TRIANGLES);
			drawVBOs (VBOSF_Quads, VBOCM_BFCBackColors, GL_QUADS);
			glDisable (GL_CULL_FACE);
		}
		else
		{
			if (cfg::RandomColors)
			{
				drawVBOs (VBOSF_Triangles, VBOCM_RandomColors, GL_TRIANGLES);
				drawVBOs (VBOSF_Quads, VBOCM_RandomColors, GL_QUADS);
			}
			else
			{
				drawVBOs (VBOSF_Triangles, VBOCM_NormalColors, GL_TRIANGLES);
				drawVBOs (VBOSF_Quads, VBOCM_NormalColors, GL_QUADS);
			}
		}

		drawVBOs (VBOSF_Lines, VBOCM_NormalColors, GL_LINES);
		glEnable (GL_LINE_STIPPLE);
		drawVBOs (VBOSF_CondLines, VBOCM_NormalColors, GL_LINES);
		glDisable (GL_LINE_STIPPLE);

		if (cfg::DrawAxes)
		{
			glBindBuffer (GL_ARRAY_BUFFER, m_axesVBO);
			glVertexPointer (3, GL_FLOAT, 0, NULL);
			glBindBuffer (GL_ARRAY_BUFFER, m_axesVBO);
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
void GLRenderer::drawVBOs (EVBOSurface surface, EVBOComplement colors, GLenum type)
{
	// Filter this through some configuration options
	if ((Eq (surface, VBOSF_Quads, VBOSF_Triangles) and cfg::DrawSurfaces == false) or
		(surface == VBOSF_Lines and cfg::DrawEdgeLines == false) or
		(surface == VBOSF_CondLines and cfg::DrawConditionalLines == false))
	{
		return;
	}

	int surfacenum = m_compiler->vboNumber (surface, VBOCM_Surfaces);
	int colornum = m_compiler->vboNumber (surface, colors);
	m_compiler->prepareVBO (surfacenum);
	m_compiler->prepareVBO (colornum);
	GLuint surfacevbo = m_compiler->vbo (surfacenum);
	GLuint colorvbo = m_compiler->vbo (colornum);
	GLsizei count = m_compiler->vboSize (surfacenum) / 3;

	if (count > 0)
	{
		glBindBuffer (GL_ARRAY_BUFFER, surfacevbo);
		glVertexPointer (3, GL_FLOAT, 0, null);
		CHECK_GL_ERROR();
		glBindBuffer (GL_ARRAY_BUFFER, colorvbo);
		glColorPointer (4, GL_FLOAT, 0, null);
		CHECK_GL_ERROR();
		glDrawArrays (type, 0, count);
		CHECK_GL_ERROR();
	}
}

// =============================================================================
// This converts a 2D point on the screen to a 3D point in the model. If 'snap'
// is true, the 3D point will snap to the current grid.
//
Vertex GLRenderer::coordconv2_3 (const QPoint& pos2d, bool snap) const
{
	if (camera() == EFreeCamera)
		return Origin;

	Vertex pos3d;
	const LDFixedCamera* cam = &g_FixedCameras[camera()];
	const Axis axisX = cam->axisX;
	const Axis axisY = cam->axisY;
	const int negXFac = cam->negX ? -1 : 1,
				negYFac = cam->negY ? -1 : 1;

	// Calculate cx and cy - these are the LDraw unit coords the cursor is at.
	double cx = (-m_virtWidth + ((2 * pos2d.x() * m_virtWidth) / m_width) - pan (X));
	double cy = (m_virtHeight - ((2 * pos2d.y() * m_virtHeight) / m_height) - pan (Y));

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
// Inverse operation for the above - convert a 3D position to a 2D screen
// position. Don't ask me how this code manages to work, I don't even know.
//
QPoint GLRenderer::coordconv3_2 (const Vertex& pos3d)
{
	GLfloat m[16];
	const LDFixedCamera* cam = &g_FixedCameras[camera()];
	const Axis axisX = cam->axisX;
	const Axis axisY = cam->axisY;
	const int negXFac = cam->negX ? -1 : 1,
				negYFac = cam->negY ? -1 : 1;

	glGetFloatv (GL_MODELVIEW_MATRIX, m);

	const double x = pos3d.x();
	const double y = pos3d.y();
	const double z = pos3d.z();

	Vertex transformed;
	transformed.setX ((m[0] * x) + (m[1] * y) + (m[2] * z) + m[3]);
	transformed.setY ((m[4] * x) + (m[5] * y) + (m[6] * z) + m[7]);
	transformed.setZ ((m[8] * x) + (m[9] * y) + (m[10] * z) + m[11]);

	double rx = (((transformed[axisX] * negXFac) + m_virtWidth + pan (X)) * m_width) / (2 * m_virtWidth);
	double ry = (((transformed[axisY] * negYFac) - m_virtHeight + pan (Y)) * m_height) / (2 * m_virtHeight);

	return QPoint (rx, -ry);
}

QPen GLRenderer::textPen() const
{
	return QPen (m_darkbg ? Qt::white : Qt::black);
}

QPen GLRenderer::linePen() const
{
	QPen linepen (m_thinBorderPen);
	linepen.setWidth (2);
	linepen.setColor (Luma (m_bgcolor) < 40 ? Qt::white : Qt::black);
	return linepen;
}

// =============================================================================
//
void GLRenderer::paintEvent (QPaintEvent*)
{
	doMakeCurrent();
	m_virtWidth = zoom();
	m_virtHeight = (m_height * m_virtWidth) / m_width;
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
		QString text = format ("Rotation: (%1, %2, %3)\nPanning: (%4, %5), Zoom: %6",
			rot(X), rot(Y), rot(Z), pan(X), pan(Y), zoom());
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

		if (overlay.img != null)
		{
			QPoint v0 = coordconv3_2 (currentDocumentData().overlays[camera()].v0),
					   v1 = coordconv3_2 (currentDocumentData().overlays[camera()].v1);

			QRect targRect (v0.x(), v0.y(), Abs (v1.x() - v0.x()), Abs (v1.y() - v0.y())),
				  srcRect (0, 0, overlay.img->width(), overlay.img->height());
			paint.drawImage (targRect, *overlay.img, srcRect);
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
		m_editmode->render (paint);

		// Draw a background for the selected camera
		paint.setPen (m_thinBorderPen);
		paint.setBrush (QBrush (QColor (0, 128, 160, 128)));
		paint.drawRect (m_cameraIcons[camera()].selRect);

		// Draw the camera icons
		for (CameraIcon& info : m_cameraIcons)
		{
			// Don't draw the free camera icon when in draw mode
			if (&info == &m_cameraIcons[EFreeCamera] and not m_editmode->allowFreeCamera())
				continue;

			paint.drawPixmap (info.destRect, *info.img, info.srcRect);
		}

		QString formatstr = tr ("%1 Camera");

		// Draw a label for the current camera in the bottom left corner
		{
			const int margin = 4;

			QString label;
			label = format (formatstr, tr (g_CameraNames[camera()]));
			paint.setPen (textPen());
			paint.drawText (QPoint (margin, height() - (margin + metrics.descent())), label);
		}

		// Tool tips
		if (m_drawToolTip)
		{
			if (not m_cameraIcons[m_toolTipCamera].destRect.contains (m_mousePosition))
				m_drawToolTip = false;
			else
			{
				QString label = format (formatstr, tr (g_CameraNames[m_toolTipCamera]));
				QToolTip::showText (m_globalpos, label);
			}
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
void GLRenderer::drawBlip (QPainter& paint, QPointF pos) const
{
	QPen pen = m_thinBorderPen;
	const int blipsize = 8;
	pen.setWidth (1);
	paint.setPen (pen);
	paint.setBrush (QColor (64, 192, 0));
	paint.drawEllipse (pos.x() - blipsize / 2, pos.y() - blipsize / 2, blipsize, blipsize);
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
	const bool wasLeft = (m_lastButtons & Qt::LeftButton) and not (ev->buttons() & Qt::LeftButton);

	Qt::MouseButtons releasedbuttons = m_lastButtons & ~ev->buttons();

	if (m_panning)
		m_panning = false;

	if (wasLeft)
	{
		// Check if we selected a camera icon
		if (not mouseHasMoved())
		{
			for (CameraIcon & info : m_cameraIcons)
			{
				if (info.destRect.contains (ev->pos()))
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
		data.keymods = m_keymods;
		data.releasedButtons = releasedbuttons;

		if (m_editmode->mouseReleased (data))
			goto end;
	}

end:
	update();
	m_totalmove = 0;
}

// =============================================================================
//
void GLRenderer::mousePressEvent (QMouseEvent* ev)
{
	m_totalmove = 0;
	m_lastButtons = ev->buttons();

	if (m_editmode->mousePressed (ev))
		ev->accept();
}

// =============================================================================
//
void GLRenderer::mouseMoveEvent (QMouseEvent* ev)
{
	int dx = ev->x() - m_mousePosition.x();
	int dy = ev->y() - m_mousePosition.y();
	m_totalmove += Abs (dx) + Abs (dy);
	setCameraMoving (false);

	if (not m_editmode->mouseMoved (ev))
	{
		const bool left = ev->buttons() & Qt::LeftButton,
				mid = ev->buttons() & Qt::MidButton,
				shift = ev->modifiers() & Qt::ShiftModifier;

		if (mid or (left and shift))
		{
			pan (X) += 0.03f * dx * (zoom() / 7.5f);
			pan (Y) -= 0.03f * dy * (zoom() / 7.5f);
			m_panning = true;
			setCameraMoving (true);
		}
		elif (left and camera() == EFreeCamera)
		{
			rot (X) = rot (X) + dy;
			rot (Y) = rot (Y) + dx;

			clampAngle (rot (X));
			clampAngle (rot (Y));
			setCameraMoving (true);
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
	m_position3D = (camera() != EFreeCamera) ? coordconv2_3 (m_mousePosition, true) : Origin;

	highlightCursorObject();
	update();
	ev->accept();
}

// =============================================================================
//
void GLRenderer::keyPressEvent (QKeyEvent* ev)
{
	m_keymods = ev->modifiers();
}

// =============================================================================
//
void GLRenderer::keyReleaseEvent (QKeyEvent* ev)
{
	m_keymods = ev->modifiers();
	m_editmode->keyReleased (ev);
	update();
}

// =============================================================================
//
void GLRenderer::wheelEvent (QWheelEvent* ev)
{
	doMakeCurrent();

	zoomNotch (ev->delta() > 0);
	zoom() = Clamp (zoom(), 0.01, 10000.0);
	setCameraMoving (true);
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
	g_win->spawnContextMenu (ev->globalPos());
}

// =============================================================================
//
void GLRenderer::setCamera (const ECamera cam)
{
	// The edit mode may forbid the free camera.
	if (cam == EFreeCamera and not m_editmode->allowFreeCamera())
		return;

	m_camera = cam;
	cfg::Camera = (int) cam;
	g_win->updateEditModeActions();
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
		LDObjectList oldsel = Selection();
		CurrentDocument()->clearSelection();

		for (LDObject* obj : oldsel)
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
	x0 = Max (0, x0);
	y0 = Max (0, y0);
	x1 = Min (x1, m_width);
	y1 = Min (y1, m_height);
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

	RemoveDuplicates (indices);

	for (qint32 idx : indices)
	{
		LDObject* obj = LDObject::fromID (idx);

		if (obj == null)
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
	g_win->updateSelection();

	// Recompile the objects now to update their color
	for (LDObject* obj : Selection())
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
	if (m_editmode != null and m_editmode->type() == a)
		return;

	delete m_editmode;
	m_editmode = AbstractEditMode::createByType (this, a);

	// If we cannot use the free camera, use the top one instead.
	if (camera() == EFreeCamera and not m_editmode->allowFreeCamera())
		setCamera (ETopCamera);

	g_win->updateEditModeActions();
	update();
}

// =============================================================================
//
EditModeType GLRenderer::currentEditModeType() const
{
	return m_editmode->type();
}

// =============================================================================
//
void GLRenderer::setDocument (LDDocument* const& a)
{
	m_document = a;

	if (a != null)
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
void GLRenderer::setPicking (const bool& a)
{
	m_isPicking = a;
	setBackground();

	if (isPicking())
	{
		glDisable (GL_DITHER);

		// Use particularly thick lines while picking ease up selecting lines.
		glLineWidth (Max<double> (cfg::LineThickness, 6.5));
	}
	else
	{
		glEnable (GL_DITHER);

		// Restore line thickness
		glLineWidth (cfg::LineThickness);
	}
}

// =============================================================================
//
void GLRenderer::getRelativeAxes (Axis& relX, Axis& relY) const
{
	const LDFixedCamera* cam = &g_FixedCameras[camera()];
	relX = cam->axisX;
	relY = cam->axisY;
}

// =============================================================================
//
Axis GLRenderer::getRelativeZ() const
{
	const LDFixedCamera* cam = &g_FixedCameras[camera()];
	return (Axis) (3 - cam->axisX - cam->axisY);
}

// =============================================================================
//
static QList<Vertex> GetVerticesOf (LDObject* obj)
{
	QList<Vertex> verts;

	if (obj->numVertices() >= 2)
	{
		for (int i = 0; i < obj->numVertices(); ++i)
			verts << obj->vertex (i);
	}
	else if (obj->type() == OBJ_Subfile)
	{
		for (LDObject* obj : static_cast<LDSubfile*> (obj)->inlineContents (true, false))
		{
			verts << GetVerticesOf (obj);
			obj->destroy();
		}
	}

	return verts;
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
	if (compiler() != null)
		compiler()->dropObject (obj);
}

// =============================================================================
//
uchar* GLRenderer::getScreencap (int& w, int& h)
{
	w = m_width;
	h = m_height;
	uchar* cap = new uchar[4 * w * h];

	m_screencap = true;
	update();
	m_screencap = false;

	// Capture the pixels
	glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, cap);

	return cap;
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
		if (icon.destRect.contains (m_mousePosition))
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
	return (y) ? cam->axisY : cam->axisX;
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
	elif (info.lh == 0)
		info.lh = (info.lw * img->height()) / img->width();

	const Axis x2d = getCameraAxis (false, cam),
		y2d = getCameraAxis (true, cam);
	const double negXFac = g_FixedCameras[cam].negX ? -1 : 1,
		negYFac = g_FixedCameras[cam].negY ? -1 : 1;

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
	info.img = null;

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
const char* GLRenderer::getCameraName() const
{
	return g_CameraNames[camera()];
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

	if (document() == null or m_width == -1 or m_height == -1)
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
	if (m_editmode->mouseDoubleClicked (ev))
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

		if (ovlobj == null and meta.img != null)
		{
			delete meta.img;
			meta.img = null;
		}
		elif (ovlobj != null and
			(meta.img == null or meta.fname != ovlobj->fileName()) and
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

		if (meta.img == null and ovlobj != null)
		{
			// If this is the last overlay image, we need to remove the empty space after it as well.
			LDObject* nextobj = ovlobj->next();

			if (nextobj and nextobj->type() == OBJ_Empty)
				nextobj->destroy();

			// If the overlay object was there and the overlay itself is
			// not, remove the object.
			ovlobj->destroy();
		}
		elif (meta.img != null and ovlobj == null)
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

		if (meta.img != null and ovlobj != null)
		{
			ovlobj->setCamera (cam);
			ovlobj->setFileName (meta.fname);
			ovlobj->setX (meta.ox);
			ovlobj->setY (meta.oy);
			ovlobj->setWidth (meta.lw);
			ovlobj->setHeight (meta.lh);
		}
	}

	if (g_win->R() == this)
		g_win->refresh();
}

// =============================================================================
//
void GLRenderer::highlightCursorObject()
{
	if (not cfg::HighlightObjectBelowCursor and objectAtCursor() == null)
		return;

	LDObject* newObject = nullptr;
	LDObject* oldObject = objectAtCursor();
	qint32 newIndex;

	if (isCameraMoving() or not cfg::HighlightObjectBelowCursor)
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

	if (newIndex != (oldObject != null ? oldObject->id() : 0))
	{
		if (newIndex != 0)
			newObject = LDObject::fromID (newIndex);

		setObjectAtCursor (newObject);

		if (oldObject != null)
			compileObject (oldObject);

		if (newObject != null)
			compileObject (newObject);
	}

	update();
}

void GLRenderer::dragEnterEvent (QDragEnterEvent* ev)
{
	if (g_win != null and ev->source() == g_win->getPrimitivesTree() and g_win->getPrimitivesTree()->currentItem() != null)
		ev->acceptProposedAction();
}

void GLRenderer::dropEvent (QDropEvent* ev)
{
	if (g_win != null and ev->source() == g_win->getPrimitivesTree())
	{
		QString primName = static_cast<SubfileListItem*> (g_win->getPrimitivesTree()->currentItem())->primitive()->name;
		LDSubfile* ref = LDSpawn<LDSubfile>();
		ref->setColor (MainColor);
		ref->setFileInfo (GetDocument (primName));
		ref->setPosition (Origin);
		ref->setTransform (IdentityMatrix);
		LDDocument::current()->insertObj (g_win->getInsertionPoint(), ref);
		ref->select();
		g_win->buildObjList();
		g_win->R()->refresh();
		ev->acceptProposedAction();
	}
}

Vertex const& GLRenderer::position3D() const
{
	return m_position3D;
}

LDFixedCamera const& GLRenderer::getFixedCamera (ECamera cam) const
{
	return g_FixedCameras[cam];
}

bool GLRenderer::mouseHasMoved() const
{
	return m_totalmove >= 10;
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
	return m_keymods;
}

LDFixedCamera const& GetFixedCamera (ECamera cam)
{
	if (cam != EFreeCamera)
		return g_FixedCameras[cam];
	else
		return g_FixedCameras[0];
}

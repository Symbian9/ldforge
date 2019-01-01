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

#define GL_GLEXT_PROTOTYPES
#include <GL/glu.h>
#include <GL/glext.h>
#include <QContextMenuEvent>
#include <QToolTip>
#include <QTimer>
#include <GL/glu.h>
#include "main.h"
#include "lddocument.h"
#include "glrenderer.h"
#include "colors.h"
#include "mainwindow.h"
#include "editHistory.h"
#include "glcompiler.h"
#include "primitives.h"
#include "documentmanager.h"
#include "grid.h"

static GLCamera const cameraTemplates[7] = {
	{"Top camera", {gl::topCameraMatrix, X, Z, false, false, false}},
	{"Front camera", {gl::frontCameraMatrix, X, Y, false,  true, false}},
	{"Left camera", {gl::leftCameraMatrix, Z, Y,  true,  true, false}},
	{"Bottom camera", {gl::bottomCameraMatrix, X, Z, false,  true, true}},
	{"Back camera", {gl::backCameraMatrix, X, Y,  true,  true, true}},
	{"Right camera", {gl::rightCameraMatrix, Z, Y, false,  true, true}},
	{"Free camera", GLCamera::FreeCamera},
};

/*
 * Constructs a GL renderer.
 */
gl::Renderer::Renderer(const Model* model, CameraType cameraType, QWidget* parent) :
    QGLWidget {parent},
    HierarchyElement {parent},
    m_model {model},
	m_camera {cameraType},
	m_cameraInfo {::cameraTemplates[cameraType]}
{
	Q_ASSERT(model != nullptr);
	m_compiler = new gl::Compiler (this);
	m_toolTipTimer = new QTimer (this);
	m_toolTipTimer->setSingleShot (true);
	setAcceptDrops (true);
	connect (m_toolTipTimer, SIGNAL (timeout()), this, SLOT (showCameraIconTooltip()));
	resetAngles();
	m_needZoomToFit = true;

	connect(
		this->m_compiler,
		&gl::Compiler::sceneChanged,
		this,
		qOverload<>(&gl::Renderer::update)
	);
}

/*
 * Destructs the GL renderer.
 */
gl::Renderer::~Renderer()
{
	freeAxes();
}

/*
 * Deletes the axes VBOs
 */
void gl::Renderer::freeAxes()
{
	if (m_axesInitialized)
	{
		glDeleteBuffers(1, &m_axesVbo);
		glDeleteBuffers(1, &m_axesColorVbo);
		m_axesInitialized = false;
	}
}

/*
 * Returns the camera currently in use.
 */
GLCamera& gl::Renderer::currentCamera()
{
	return m_cameraInfo;
}

/*
 * Returns the camera currently in use.
 */
const GLCamera& gl::Renderer::currentCamera() const
{
	return m_cameraInfo;
}

/*
 * Prepares the GL context for rendering.
 */
void gl::Renderer::initGLData()
{
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1.0f, 1.0f);
	glEnable (GL_DEPTH_TEST);
	glShadeModel (GL_SMOOTH);
	glEnable (GL_MULTISAMPLE);

	if (config::antiAliasedLines())
	{
		glEnable (GL_LINE_SMOOTH);
		glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
	}
	else
	{
		glDisable (GL_LINE_SMOOTH);
	}
}

/*
 * Returns the object currently highlighted by the cursor.
 */
QPersistentModelIndex gl::Renderer::objectAtCursor() const
{
	return m_objectAtCursor;
}

// =============================================================================
//
void gl::Renderer::needZoomToFit()
{
	m_needZoomToFit = true;
}

// =============================================================================
//
void gl::Renderer::resetAngles()
{
	if (m_initialized)
	{
		m_rotation = QQuaternion::fromAxisAndAngle({1, 0, 0}, 30);
		m_rotation *= QQuaternion::fromAxisAndAngle({0, 1, 0}, 330);
	}
	currentCamera().setPanning(0, 0);
	needZoomToFit();
}

// =============================================================================
//
void gl::Renderer::initializeGL()
{
	initializeOpenGLFunctions();

	if (glGetError() != GL_NO_ERROR)
	{
		abort();
	}

	setBackground();
	glLineWidth (config::lineThickness());
	glLineStipple (1, 0x6666);
	setAutoFillBackground (false);
	setMouseTracking (true);
	setFocusPolicy (Qt::WheelFocus);
	m_compiler->initialize();
	initializeAxes();
	initializeLighting();
	m_initialized = true;
	// Now that GL is initialized, we can reset angles.
	resetAngles();
}

void gl::Renderer::initializeLighting()
{
	GLfloat materialShininess[] = {5.0};
	GLfloat lightPosition[] = {1.0, 1.0, 1.0, 0.0};
	GLfloat ambientLightingLevel[] = {0.5, 0.5, 0.5, 1.0};
	glShadeModel(GL_SMOOTH);
	glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLightingLevel);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, ambientLightingLevel);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
}

// =============================================================================
//
void gl::Renderer::initializeAxes()
{
	freeAxes();
	float axisVertexData[3][6];
	float axisColorData[3][6];

	auto compileAxis = [&](Axis axis, QColor color, Vertex extrema)
	{
		axisVertexData[axis][0] = extrema[X];
		axisVertexData[axis][1] = extrema[Y];
		axisVertexData[axis][2] = extrema[Z];
		axisVertexData[axis][3] = -extrema[X];
		axisVertexData[axis][4] = -extrema[Y];
		axisVertexData[axis][5] = -extrema[Z];
		axisColorData[axis][0] = axisColorData[axis][3] = color.red();
		axisColorData[axis][1] = axisColorData[axis][4] = color.green();
		axisColorData[axis][2] = axisColorData[axis][5] = color.blue();
	};

	compileAxis(X, {192, 96, 96}, {10000, 0, 0});
	compileAxis(Y, {48, 192, 48}, {0, 10000, 0});
	compileAxis(Z, {48, 112, 192}, {0, 0, 10000});

	glGenBuffers(1, &m_axesVbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_axesVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof axisVertexData, axisVertexData, GL_STATIC_DRAW);
	glGenBuffers(1, &m_axesColorVbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_axesColorVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof axisColorData, axisColorData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// =============================================================================
//
void gl::Renderer::setBackground()
{
	if (not m_isDrawingSelectionScene)
	{
		// Otherwise use the background that the user wants.
		QColor color = config::backgroundColor();

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

QColor gl::Renderer::backgroundColor() const
{
	return m_backgroundColor;
}

// =============================================================================
//
void gl::Renderer::resizeGL (int width, int height)
{
	glViewport (0, 0, width, height);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluPerspective (45.0f, (double) width / (double) height, near, far);
	glMatrixMode (GL_MODELVIEW);

	// Unfortunately Qt does not provide a resized() signal so we have to manually feed the information.
	m_cameraInfo.rendererResized(width, height);
}

/*
 * Pads a 3×3 matrix into a 4×4 one by adding cells from the identity matrix.
 */
QMatrix4x4 padMatrix(const QMatrix3x3& stub)
{
	return {
		stub(0, 0), stub(0, 1), stub(0, 2), 0,
		stub(1, 0), stub(1, 1), stub(1, 2), 0,
		stub(2, 0), stub(2, 1), stub(2, 2), 0,
		0, 0, 0, 1
	};
}

// =============================================================================
//
void gl::Renderer::drawGLScene()
{
	if (m_needZoomToFit)
	{
		m_needZoomToFit = false;
		zoomAllToFit();
	}

	if (config::drawWireframe() and not m_isDrawingSelectionScene)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (config::lighting() and not m_isDrawingSelectionScene)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);

	if (not m_cameraInfo.isModelview())
	{
		glMatrixMode (GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glMultMatrixf(currentCamera().realMatrix().constData());
		glMultMatrixf(ldrawToGLAdapterMatrix.constData());
		drawFixedCameraBackdrop();
	}
	else
	{
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, -2.0f);
		glTranslatef(panning (X), panning (Y), -zoom());
		glMultMatrixf(padMatrix(m_rotation.toRotationMatrix()).constData());
		xyz(glTranslatef, -m_compiler->modelCenter());
	}

	glEnableClientState (GL_NORMAL_ARRAY);
	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);

	if (m_isDrawingSelectionScene)
	{
		drawVbos (VboClass::Triangles, VboSubclass::PickColors);
		drawVbos (VboClass::Quads, VboSubclass::PickColors);
		drawVbos (VboClass::Lines, VboSubclass::PickColors);
		drawVbos (VboClass::ConditionalLines, VboSubclass::PickColors);
	}
	else
	{
		if (config::bfcRedGreenView())
		{
			glEnable (GL_CULL_FACE);
			glCullFace (GL_BACK);
			drawVbos (VboClass::Triangles, VboSubclass::BfcFrontColors);
			drawVbos (VboClass::Quads, VboSubclass::BfcFrontColors);
			glCullFace (GL_FRONT);
			drawVbos (VboClass::Triangles, VboSubclass::BfcBackColors);
			drawVbos (VboClass::Quads, VboSubclass::BfcBackColors);
			glDisable (GL_CULL_FACE);
		}
		else
		{
			VboSubclass colors;

			if (config::randomColors())
				colors = VboSubclass::RandomColors;
			else
				colors = VboSubclass::RegularColors;

			drawVbos (VboClass::Triangles, colors);
			drawVbos (VboClass::Quads, colors);
		}

		drawVbos (VboClass::Lines, VboSubclass::RegularColors);

		if (config::useLineStipple())
			glEnable (GL_LINE_STIPPLE);

		drawVbos (VboClass::ConditionalLines, VboSubclass::RegularColors);
		glDisable (GL_LINE_STIPPLE);

		if (config::drawAxes())
		{
			glDisableClientState (GL_NORMAL_ARRAY);
			glBindBuffer (GL_ARRAY_BUFFER, m_axesVbo);
			glVertexPointer (3, GL_FLOAT, 0, NULL);
			glBindBuffer (GL_ARRAY_BUFFER, m_axesVbo);
			glColorPointer (3, GL_FLOAT, 0, NULL);
			glDrawArrays (GL_LINES, 0, 6);
			glEnableClientState (GL_NORMAL_ARRAY);
			CHECK_GL_ERROR();
		}
	}

	glPopMatrix();
	glBindBuffer (GL_ARRAY_BUFFER, 0);
	glDisableClientState (GL_VERTEX_ARRAY);
	glDisableClientState (GL_COLOR_ARRAY);
	glDisableClientState (GL_NORMAL_ARRAY);
	CHECK_GL_ERROR();
	glDisable (GL_CULL_FACE);
	glMatrixMode (GL_MODELVIEW);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
}

/*
 * Draws a set of VBOs onto the scene. Renders surfaces with appropriate normals and colors.
 *
 * Parameters:
 * - surface determines what kind of surface to draw (triangles, quadrilaterals, edges or conditional edges)
 * - colors determines what VBO subclass to use for colors
 */
void gl::Renderer::drawVbos(VboClass surface, VboSubclass colors)
{
	// Filter this through some configuration options
	if ((isOneOf(surface, VboClass::Quads, VboClass::Triangles) and config::drawSurfaces() == false)
		or (surface == VboClass::Lines and config::drawEdgeLines() == false)
		or (surface == VboClass::ConditionalLines and config::drawConditionalLines() == false))
	{
		return;
	}

	GLenum type;

	switch (surface)
	{
	case VboClass::_End:
	case VboClass::Lines:
	case VboClass::ConditionalLines:
		type = GL_LINES;
		break;

	case VboClass::Triangles:
		type = GL_TRIANGLES;
		break;

	case VboClass::Quads:
		type = GL_QUADS;
		break;
	}

	VboSubclass normals;

	if (colors != VboSubclass::BfcBackColors)
		normals = VboSubclass::Normals;
	else
		normals = VboSubclass::InvertedNormals;

	int surfaceVboNumber = m_compiler->vboNumber(surface, VboSubclass::Surfaces);
	int colorVboNumber = m_compiler->vboNumber(surface, colors);
	int normalVboNumber = m_compiler->vboNumber(surface, normals);
	m_compiler->prepareVBO(surfaceVboNumber);
	m_compiler->prepareVBO(colorVboNumber);
	m_compiler->prepareVBO(normalVboNumber);
	GLuint surfaceVbo = m_compiler->vbo(surfaceVboNumber);
	GLuint colorVbo = m_compiler->vbo(colorVboNumber);
	GLuint normalVbo = m_compiler->vbo(normalVboNumber);
	GLsizei count = m_compiler->vboSize(surfaceVboNumber) / 3;

	if (count > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, surfaceVbo);
		glVertexPointer(3, GL_FLOAT, 0, nullptr);
		CHECK_GL_ERROR();
		glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
		glColorPointer(4, GL_FLOAT, 0, nullptr);
		CHECK_GL_ERROR();
		glBindBuffer(GL_ARRAY_BUFFER, normalVbo);
		glNormalPointer(GL_FLOAT, 0, nullptr);
		CHECK_GL_ERROR();
		glDrawArrays(type, 0, count);
		CHECK_GL_ERROR();
	}
}

QPen gl::Renderer::textPen() const
{
	return {m_useDarkBackground ? Qt::white : Qt::black};
}

bool gl::Renderer::freeCameraAllowed() const
{
	return true;
}

void gl::Renderer::paintEvent(QPaintEvent*)
{
	makeCurrent();
	initGLData();
	drawGLScene();

	if (isDrawingSelectionScene())
		return;

	QPainter painter {this};
	painter.setRenderHint(QPainter::Antialiasing);
	overpaint(painter);
}

void gl::Renderer::overpaint(QPainter &painter)
{
	// Draw a label for the current camera in the bottom left corner
	{
		QFontMetrics metrics {QFont {}};
		int margin = 4;
		painter.setPen(textPen());
		painter.drawText(QPoint {margin, height() - margin - metrics.descent()}, currentCamera().name());
	}
}

// =============================================================================
//
void gl::Renderer::mouseReleaseEvent(QMouseEvent* event)
{
	ignore(event);
	m_panning = false;
	update();
	m_totalMouseMove = 0;
}

// =============================================================================
//
void gl::Renderer::mousePressEvent(QMouseEvent* event)
{
	m_lastButtons = event->buttons();
	m_totalMouseMove = 0;
}

// =============================================================================
//
void gl::Renderer::mouseMoveEvent(QMouseEvent* event)
{
	int xMove = event->x() - m_mousePosition.x();
	int yMove = event->y() - m_mousePosition.y();
	m_totalMouseMove += qAbs(xMove) + qAbs(yMove);
	m_isCameraMoving = false;

	bool left = event->buttons() & Qt::LeftButton;
	bool mid = event->buttons() & Qt::MidButton;
	bool shift = event->modifiers() & Qt::ShiftModifier;

	if (mid or (left and shift))
	{
		currentCamera().pan(xMove, yMove);
		m_panning = true;
		m_isCameraMoving = true;
	}
	else if (left and m_cameraInfo.isModelview() and (xMove != 0 or yMove != 0))
	{
		QQuaternion versor = QQuaternion::fromAxisAndAngle(yMove, xMove, 0, 0.6 * hypot(xMove, yMove));
		m_rotation = versor * m_rotation;
		m_isCameraMoving = true;
	}

	// Start the tool tip timer
	m_toolTipTimer->start (500);

	// Update 2d position
	m_mousePosition = event->pos();
	m_globalpos = event->globalPos();
	m_mousePositionF = event->localPos();

	highlightCursorObject();
	update();
	event->accept();
}

// =============================================================================
//
void gl::Renderer::keyPressEvent(QKeyEvent* event)
{
	m_currentKeyboardModifiers = event->modifiers();
}

// =============================================================================
//
void gl::Renderer::keyReleaseEvent(QKeyEvent* event)
{
	m_currentKeyboardModifiers = event->modifiers();
	update();
}

// =============================================================================
//
void gl::Renderer::wheelEvent(QWheelEvent* ev)
{
	makeCurrent();
	currentCamera().zoomNotch(ev->delta() > 0);
	m_isCameraMoving = true;
	update();
	ev->accept();
}

// =============================================================================
//
void gl::Renderer::leaveEvent(QEvent*)
{
	m_toolTipTimer->stop();
	update();
}

/*
 * Resolves a pixel pointer to an RGB color.
 * pixel[0..2] must be valid.
 */
static QRgb colorFromPixel(uint8_t* pixel)
{
	return pixel[0] << 16 | pixel[1] << 8 | pixel[2] | 0xff000000;
}

/*
 * Returns the set of objects found in the specified pixel area.
 */
QItemSelection gl::Renderer::pick(const QRect& range)
{
	makeCurrent();
	QItemSelection result;

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
	x1 = qMin (x1, width());
	y1 = qMin (y1, height());
	const int areawidth = (x1 - x0);
	const int areaheight = (y1 - y0);
	const qint32 numpixels = areawidth * areaheight;

	// Allocate space for the pixel data.
	QVector<unsigned char> pixelData;
	pixelData.resize(4 * numpixels);

	// Read pixels from the color buffer.
	glReadPixels(x0, height() - y1, areawidth, areaheight, GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data());

	QSet<QRgb> pixelColors;

	// Go through each pixel read and add them to the selection.
	// Each pixel maps to an LDObject injectively.
	// Note: black is background, those indices are skipped.
	for (int i : ::range(0, 4, pixelData.size() - 4))
	{
		QRgb color = colorFromPixel(&pixelData[i]);

		if (color != BlackRgb)
			pixelColors.insert(color);
	}

	// For each index read, resolve the LDObject behind it and add it to the selection.
	for (QRgb color : pixelColors)
	{
		QModelIndex index = m_model->objectByPickingColor(color);

		if (index.isValid())
			result.select(index, index);
	}

	setPicking(false);
	repaint();
	return result;
}

/*
 * Simpler version of gl::Renderer::pick which simply picks whatever object on the cursor
 */
QModelIndex gl::Renderer::pick(int mouseX, int mouseY)
{
	makeCurrent();
	setPicking(true);
	drawGLScene();
	unsigned char pixel[4];
	glReadPixels(mouseX, height() - mouseY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	QModelIndex result = m_model->objectByPickingColor(colorFromPixel(pixel));
	setPicking(false);
	repaint();
	return result;
}

// =============================================================================
//
void gl::Renderer::setPicking(bool picking)
{
	m_isDrawingSelectionScene = picking;
	setBackground();

	if (m_isDrawingSelectionScene)
	{
		glDisable(GL_DITHER);

		// Use particularly thick lines while picking ease up selecting lines.
		glLineWidth(qMax<double>(config::lineThickness(), 6.5));
	}
	else
	{
		glEnable(GL_DITHER);

		// Restore line thickness
		glLineWidth(config::lineThickness());
	}
}

/*
 * Returns an image containing the current render of the scene.
 */
QImage gl::Renderer::screenCapture()
{
	// Read the current render to a buffer of pixels. We use RGBA format even though the image should be fully opaque at all times.
	// This is because apparently GL_RGBA/GL_UNSIGNED_BYTE is the only setting pair that is guaranteed to actually work!
	// ref: https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glReadPixels.xml
	QVector<unsigned char> pixelData;
	pixelData.resize(4 * width() * height());
	glReadPixels(0, 0, width(), height(), GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data());

	// Prepare the image and return it. It appears that GL and Qt formats have red and blue swapped and the Y axis flipped.
	QImage image {pixelData.constData(), width(), height(), QImage::Format_ARGB32};
	return image.rgbSwapped().mirrored();
}

// =============================================================================
//
void gl::Renderer::zoomToFit()
{
	currentCamera().setZoom(30.0f);
	bool lastfilled = false;
	bool firstrun = true;
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
			currentCamera().setZoom(30.0);
			break;
		}

		currentCamera().zoomNotch(inward);
		QVector<unsigned char> capture (4 * width() * height());
		drawGLScene();
		glReadPixels (0, 0, width(), height(), GL_RGBA, GL_UNSIGNED_BYTE, capture.data());
		QImage image (capture.constData(), width(), height(), QImage::Format_ARGB32);
		bool filled = false;

		// Check the top and bottom rows
		for (int i = 0; i < image.width(); ++i)
		{
			if (image.pixel (i, 0) != BlackRgb or image.pixel (i, height() - 1) != BlackRgb)
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
				if (image.pixel (0, i) != BlackRgb or image.pixel (width() - 1, i) != BlackRgb)
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
				currentCamera().zoomNotch(false);
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
void gl::Renderer::zoomAllToFit()
{
	zoomToFit();
}

// =============================================================================
//
void gl::Renderer::highlightCursorObject()
{
	if (not config::highlightObjectBelowCursor() and not objectAtCursor().isValid())
		return;

	QModelIndex newIndex;
	QModelIndex oldIndex = m_objectAtCursor;

	if (not m_isCameraMoving and config::highlightObjectBelowCursor())
	{
		setPicking (true);
		drawGLScene();
		setPicking (false);
		unsigned char pixel[4];
		glReadPixels(
			m_mousePosition.x(),
			height() - m_mousePosition.y(),
			1,
			1,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			&pixel[0]
		);
		newIndex = model()->objectByPickingColor(colorFromPixel(pixel));
	}

	if (newIndex != oldIndex)
	{
		m_objectAtCursor = newIndex;
		emit objectHighlightingChanged(oldIndex, newIndex);
	}

	update();
}

bool gl::Renderer::mouseHasMoved() const
{
	return m_totalMouseMove >= 10;
}

QPoint const& gl::Renderer::mousePosition() const
{
	return m_mousePosition;
}

QPointF const& gl::Renderer::mousePositionF() const
{
	return m_mousePositionF;
}

Qt::KeyboardModifiers gl::Renderer::keyboardModifiers() const
{
	return m_currentKeyboardModifiers;
}

gl::CameraType gl::Renderer::camera() const
{
	return m_camera;
}

double gl::Renderer::panning (Axis ax) const
{
	return (ax == X) ? currentCamera().panningX() : currentCamera().panningY();
}

double gl::Renderer::zoom()
{
	return currentCamera().zoom();
}

bool gl::Renderer::isDrawingSelectionScene() const
{
	return m_isDrawingSelectionScene;
}

Qt::MouseButtons gl::Renderer::lastButtons() const
{
	return m_lastButtons;
}

const Model* gl::Renderer::model() const
{
	return m_model;
}

/*
 * This virtual function lets derivative classes render something to the fixed camera
 * before the main brick is rendered.
 */
void gl::Renderer::drawFixedCameraBackdrop() {}

QItemSelectionModel* gl::Renderer::selectionModel() const
{
	return m_compiler->selectionModel();
}

void gl::Renderer::setSelectionModel(QItemSelectionModel* selectionModel)
{
	this->m_compiler->setSelectionModel(selectionModel);
}

void gl::Renderer::fullUpdate()
{
	this->m_compiler->fullUpdate();
	update();
}

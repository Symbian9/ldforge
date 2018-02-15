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
#include "lddocument.h"
#include "glrenderer.h"
#include "colors.h"
#include "mainwindow.h"
#include "miscallenous.h"
#include "editHistory.h"
#include "glcompiler.h"
#include "primitives.h"
#include "documentmanager.h"
#include "grid.h"

const QPen GLRenderer::thinBorderPen {QColor {0, 0, 0, 208}, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin};

// Transformation matrices for the fixed cameras.
const GLRotationMatrix GLRenderer::topCameraMatrix = GLRotationMatrix {};
const GLRotationMatrix GLRenderer::frontCameraMatrix = {1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1};
const GLRotationMatrix GLRenderer::leftCameraMatrix = {0, -1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0, 0, 0, 1};
const GLRotationMatrix GLRenderer::bottomCameraMatrix = {1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1};
const GLRotationMatrix GLRenderer::backCameraMatrix = {-1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1};
const GLRotationMatrix GLRenderer::rightCameraMatrix = {0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1};

// Conversion matrix from LDraw to OpenGL coordinates.
const GLRotationMatrix GLRenderer::ldrawToGLAdapterMatrix = {1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1};

/*
 * Constructs a GL renderer.
 */
GLRenderer::GLRenderer(const Model* model, QWidget* parent) :
    QGLWidget {parent},
    HierarchyElement {parent},
    m_model {model},
    m_cameras {
        {"Top camera", {topCameraMatrix, X, Z, false, false, false}}, // top
        {"Front camera", {frontCameraMatrix, X, Y, false,  true, false}}, // front
        {"Left camera", {leftCameraMatrix, Z, Y,  true,  true, false}}, // left
        {"Bottom camera", {bottomCameraMatrix, X, Z, false,  true, true}}, // bottom
        {"Back camera", {backCameraMatrix, X, Y,  true,  true, true}}, // back
        {"Right camera", {rightCameraMatrix, Z, Y, false,  true, true}}, // right
        {"Free camera", GLCamera::FreeCamera}, // free
    }
{
	m_camera = (Camera) m_config->camera();
	m_compiler = new GLCompiler (this);
	m_toolTipTimer = new QTimer (this);
	m_toolTipTimer->setSingleShot (true);
	setAcceptDrops (true);
	connect (m_toolTipTimer, SIGNAL (timeout()), this, SLOT (showCameraIconTooltip()));
	resetAllAngles();
	m_needZoomToFit = true;

	// Init camera icons
	for (Camera camera : iterateEnum<Camera>())
	{
		const char* cameraIconNames[EnumLimits<Camera>::Count] =
		{
		    "camera-top", "camera-front", "camera-left",
		    "camera-bottom", "camera-back", "camera-right",
		    "camera-free"
		};

		CameraIcon* info = &m_cameraIcons[static_cast<int>(camera)];
		info->image = MainWindow::getIcon (cameraIconNames[static_cast<int>(camera)]);
		info->camera = camera;
	}

	calcCameraIcons();
}

/*
 * Destructs the GL renderer.
 */
GLRenderer::~GLRenderer()
{
	freeAxes();
}

/*
 * Deletes the axes VBOs
 */
void GLRenderer::freeAxes()
{
	if (m_axesInitialized)
	{
		glDeleteBuffers(1, &m_axesVbo);
		glDeleteBuffers(1, &m_axesColorVbo);
		m_axesInitialized = false;
	}
}

/*
 * Calculates the camera icon locations.
 */
void GLRenderer::calcCameraIcons()
{
	int i = 0;
	const int columns = 3;
	const int firstAtLastRow = countof(m_cameras) - (countof(m_cameras) % columns);

	for (CameraIcon& cameraIcon : m_cameraIcons)
	{
		int row = i / columns;
		int column = i % columns;

		// Do right-justifying on the last row.
		if (i >= firstAtLastRow)
			column += columns - (countof(m_cameras) % columns);

		int x1 = width() - 48 + (column * 16) - 1;
		int y1 = (row * 16) + 1;

		cameraIcon.sourceRect = {0, 0, 16, 16};
		cameraIcon.targetRect = {x1, y1, 16, 16};
		cameraIcon.hitRect = {
		    cameraIcon.targetRect.x(),
		    cameraIcon.targetRect.y(),
		    cameraIcon.targetRect.width() + 1,
		    cameraIcon.targetRect.height() + 1
		};

		++i;
	}
}

/*
 * Returns the camera currently in use.
 */
GLCamera& GLRenderer::currentCamera()
{
	return m_cameras[static_cast<int>(camera())];
}

/*
 * Returns the camera currently in use.
 */
const GLCamera& GLRenderer::currentCamera() const
{
	return m_cameras[static_cast<int>(camera())];
}

/*
 * Prepares the GL context for rendering.
 */
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
QPersistentModelIndex GLRenderer::objectAtCursor() const
{
	return m_objectAtCursor;
}

// =============================================================================
//
void GLRenderer::needZoomToFit()
{
	m_needZoomToFit = true;
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
		glGetFloatv(GL_MODELVIEW_MATRIX, m_rotationMatrix.data());
		glPopMatrix();
	}
	currentCamera().setPanning(0, 0);
	needZoomToFit();
}

// =============================================================================
//
void GLRenderer::resetAllAngles()
{
	Camera oldCamera = camera();

	for (Camera camera : iterateEnum<Camera>())
	{
		setCamera(camera);
		resetAngles();
	}

	setCamera(oldCamera);
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
	m_compiler->initialize();
	initializeAxes();
	initializeLighting();
	m_initialized = true;
	// Now that GL is initialized, we can reset angles.
	resetAllAngles();
}

void GLRenderer::initializeLighting()
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
void GLRenderer::initializeAxes()
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

QColor GLRenderer::backgroundColor() const
{
	return m_backgroundColor;
}

// =============================================================================
//
void GLRenderer::resizeGL (int width, int height)
{
	calcCameraIcons();
	glViewport (0, 0, width, height);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluPerspective (45.0f, (double) width / (double) height, 1.0f, 10000.0f);
	glMatrixMode (GL_MODELVIEW);

	// Unfortunately Qt does not provide a resized() signal so we have to manually feed the information.
	for (GLCamera& camera : m_cameras)
		camera.rendererResized(width, height);
}

// =============================================================================
//
void GLRenderer::drawGLScene()
{
	if (m_needZoomToFit)
	{
		m_needZoomToFit = false;
		zoomAllToFit();
	}

	if (m_config->drawWireframe() and not m_isDrawingSelectionScene)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (m_config->lighting() and not m_isDrawingSelectionScene)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);

	if (camera() != Camera::Free)
	{
		glMatrixMode (GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glMultMatrixf(currentCamera().realMatrix());
		glMultMatrixf(ldrawToGLAdapterMatrix);
		drawFixedCameraBackdrop();
	}
	else
	{
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, -2.0f);
		glTranslatef(panning (X), panning (Y), -zoom());
		glMultMatrixf(m_rotationMatrix.constData());
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
		if (m_config->bfcRedGreenView())
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

			if (m_config->randomColors())
				colors = VboSubclass::RandomColors;
			else
				colors = VboSubclass::RegularColors;

			drawVbos (VboClass::Triangles, colors);
			drawVbos (VboClass::Quads, colors);
		}

		drawVbos (VboClass::Lines, VboSubclass::RegularColors);
		glEnable (GL_LINE_STIPPLE);
		drawVbos (VboClass::ConditionalLines, VboSubclass::RegularColors);
		glDisable (GL_LINE_STIPPLE);

		if (m_config->drawAxes())
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
void GLRenderer::drawVbos(VboClass surface, VboSubclass colors)
{
	// Filter this through some configuration options
	if ((isOneOf(surface, VboClass::Quads, VboClass::Triangles) and m_config->drawSurfaces() == false)
		or (surface == VboClass::Lines and m_config->drawEdgeLines() == false)
		or (surface == VboClass::ConditionalLines and m_config->drawConditionalLines() == false))
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

QPen GLRenderer::textPen() const
{
	return {m_useDarkBackground ? Qt::white : Qt::black};
}

bool GLRenderer::freeCameraAllowed() const
{
	return true;
}

void GLRenderer::paintEvent(QPaintEvent*)
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

void GLRenderer::overpaint(QPainter &painter)
{
	// Draw a background for the selected camera
	painter.setPen(thinBorderPen);
	painter.setBrush(QBrush {QColor {0, 128, 160, 128}});
	painter.drawRect(m_cameraIcons[static_cast<int>(camera())].hitRect);

	// Draw the camera icons
	for (const CameraIcon& info : m_cameraIcons)
	{
		// Don't draw the free camera icon when we can't use the free camera
		if (info.camera == Camera::Free and not freeCameraAllowed())
			continue;

		painter.drawPixmap(info.targetRect, info.image, info.sourceRect);
	}

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
void GLRenderer::mouseReleaseEvent(QMouseEvent* event)
{
	bool wasLeft = (m_lastButtons & Qt::LeftButton) and not (event->buttons() & Qt::LeftButton);
	m_panning = false;

	// Check if we selected a camera icon
	if (wasLeft and not mouseHasMoved())
	{
		for (CameraIcon& info : m_cameraIcons)
		{
			if (info.targetRect.contains (event->pos()))
			{
				setCamera (info.camera);
				break;
			}
		}
	}

	update();
	m_totalMouseMove = 0;
}

// =============================================================================
//
void GLRenderer::mousePressEvent(QMouseEvent* event)
{
	m_lastButtons = event->buttons();
	m_totalMouseMove = 0;
}

// =============================================================================
//
void GLRenderer::mouseMoveEvent(QMouseEvent* event)
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
	else if (left and camera() == Camera::Free and (xMove != 0 or yMove != 0))
	{
		// Apply current rotation input to the rotation matrix
		// ref: https://forums.ldraw.org/thread-22006-post-24426.html#pid24426
		glPushMatrix();
		glLoadIdentity();
		// 0.6 is an arbitrary rotation sensitivity scalar
		glRotatef(0.6 * hypot(xMove, yMove), yMove, xMove, 0);
		glMultMatrixf(m_rotationMatrix.constData());
		glGetFloatv(GL_MODELVIEW_MATRIX, m_rotationMatrix.data());
		glPopMatrix();
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
void GLRenderer::keyPressEvent(QKeyEvent* event)
{
	m_currentKeyboardModifiers = event->modifiers();
}

// =============================================================================
//
void GLRenderer::keyReleaseEvent(QKeyEvent* event)
{
	m_currentKeyboardModifiers = event->modifiers();
	update();
}

// =============================================================================
//
void GLRenderer::wheelEvent(QWheelEvent* ev)
{
	makeCurrent();
	currentCamera().zoomNotch(ev->delta() > 0);
	m_isCameraMoving = true;
	update();
	ev->accept();
}

// =============================================================================
//
void GLRenderer::leaveEvent(QEvent*)
{
	m_toolTipTimer->stop();
	update();
}

// =============================================================================
//
void GLRenderer::setCamera(Camera camera)
{
	// The edit mode may forbid the free camera.
	if (freeCameraAllowed() or camera != Camera::Free)
	{
		m_camera = camera;
		m_config->setCamera(static_cast<int>(camera));
	}
}

/*
 * Returns the set of objects found in the specified pixel area.
 */
QSet<LDObject*> GLRenderer::pick(const QRect& range)
{
	makeCurrent();
	QSet<LDObject*> newSelection;

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

	QSet<qint32> indices;

	// Go through each pixel read and add them to the selection.
	// Each pixel maps to an LDObject index injectively.
	// Note: black is background, those indices are skipped.
	for (unsigned char *pixelCursor = pixelData.begin(); pixelCursor < pixelData.end(); pixelCursor += 4)
	{
		qint32 index = pixelCursor[0] * 0x10000 + pixelCursor[1] * 0x100 + pixelCursor[2] * 0x1;
		if (index != 0)
			indices.insert(index);
	}

	// For each index read, resolve the LDObject behind it and add it to the selection.
	for (qint32 index : indices)
	{
		LDObject* object = LDObject::fromID(index);

		if (object != nullptr)
			newSelection.insert(object);
	}

	setPicking(false);
	repaint();
	return newSelection;
}

/*
 * Simpler version of GLRenderer::pick which simply picks whatever object on the cursor
 */
LDObject* GLRenderer::pick(int mouseX, int mouseY)
{
	unsigned char pixel[4];
	makeCurrent();
	setPicking(true);
	drawGLScene();
	glReadPixels(mouseX, height() - mouseY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	LDObject* object = LDObject::fromID(pixel[0] * 0x10000 + pixel[1] * 0x100 + pixel[2]);
	setPicking(false);
	repaint();
	return object;
}

// =============================================================================
//
void GLRenderer::setPicking(bool picking)
{
	m_isDrawingSelectionScene = picking;
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

/*
 * Returns an image containing the current render of the scene.
 */
QImage GLRenderer::screenCapture()
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

/*
 * Show a tooltip if the cursor is currently hovering over a camera icon.
 */
void GLRenderer::showCameraIconTooltip()
{
	for (CameraIcon & icon : m_cameraIcons)
	{
		if (icon.targetRect.contains (m_mousePosition))
		{
			QToolTip::showText(m_globalpos, m_cameras[static_cast<int>(icon.camera)].name());
			update();
			break;
		}
	}
}

// =============================================================================
//
void GLRenderer::zoomToFit()
{
	currentCamera().setZoom(30.0f);
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
			if (image.pixel (i, 0) != black or image.pixel (i, height() - 1) != black)
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
				if (image.pixel (0, i) != black or image.pixel (width() - 1, i) != black)
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
void GLRenderer::zoomAllToFit()
{
	zoomToFit();
}

// =============================================================================
//
void GLRenderer::highlightCursorObject()
{
	if (not m_config->highlightObjectBelowCursor() and not objectAtCursor().isValid())
		return;

	QModelIndex newIndex;
	QModelIndex oldIndex = m_objectAtCursor;

	if (not m_isCameraMoving and m_config->highlightObjectBelowCursor())
	{
		setPicking (true);
		drawGLScene();
		setPicking (false);

		unsigned char pixel[4];
		glReadPixels (m_mousePosition.x(), height() - m_mousePosition.y(), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel[0]);
		qint32 id = pixel[0] * 0x10000 | pixel[1] * 0x100 | pixel[2];

		if (id != 0)
			newIndex = model()->indexFromId(id);
	}

	if (newIndex != oldIndex)
	{
		m_objectAtCursor = newIndex;
		emit objectHighlightingChanged(oldIndex, newIndex);
	}

	update();
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

Qt::KeyboardModifiers GLRenderer::keyboardModifiers() const
{
	return m_currentKeyboardModifiers;
}

Camera GLRenderer::camera() const
{
	return m_camera;
}

double GLRenderer::panning (Axis ax) const
{
	return (ax == X) ? currentCamera().panningX() : currentCamera().panningY();
}

double GLRenderer::zoom()
{
	return currentCamera().zoom();
}

const QGenericMatrix<4, 4, GLfloat>& GLRenderer::rotationMatrix() const
{
	return m_rotationMatrix;
}

bool GLRenderer::isDrawingSelectionScene() const
{
	return m_isDrawingSelectionScene;
}

Qt::MouseButtons GLRenderer::lastButtons() const
{
	return m_lastButtons;
}

const Model* GLRenderer::model() const
{
	return m_model;
}

/*
 * This virtual function lets derivative classes render something to the fixed camera
 * before the main brick is rendered.
 */
void GLRenderer::drawFixedCameraBackdrop() {}

void GLRenderer::setSelectionModel(QItemSelectionModel* selectionModel)
{
	this->m_compiler->setSelectionModel(selectionModel);
}

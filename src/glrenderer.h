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

#pragma once
#include <QGLWidget>
#include "main.h"
#include "model.h"
#include "glShared.h"
#include "glcamera.h"
#include "hierarchyelement.h"

class GLCompiler;
class MessageManager;
class QDialogButtonBox;
class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;
class QTimer;
class MagicWandMode;

struct CameraInfo
{
	CameraInfo(const CameraInfo&) = delete;

	int glrotate[3];
	Axis localX;
	Axis localY;
	bool negatedX;
	bool negatedY;
	bool negatedDepth; // is greater depth value closer to camera?
};

enum class Camera
{
	Top,
	Front,
	Left,
	Bottom,
	Back,
	Right,
	Free,
	_End
};

struct CameraIcon
{
	QPixmap image;
	QRect sourceRect;
	QRect targetRect;
	QRect hitRect;
	Camera camera;
};

MAKE_ITERABLE_ENUM(Camera)

// The main renderer object, draws the brick on the screen, manages the camera and selection picking.
class GLRenderer : public QGLWidget, protected QOpenGLFunctions, public HierarchyElement
{
	Q_OBJECT

public:
	GLRenderer(const Model* model, QWidget* parent = nullptr);
	~GLRenderer();

	QColor backgroundColor() const;
	Camera camera() const;
	QByteArray capturePixels();
	GLCamera& currentCamera();
	const GLCamera& currentCamera() const;
	void drawGLScene();
	void highlightCursorObject();
	void initGLData();
	bool isPicking() const;
	Qt::KeyboardModifiers keyboardModifiers() const;
	const Model* model() const;
	bool mouseHasMoved() const;
	QPoint const& mousePosition() const;
	QPointF const& mousePositionF() const;
	void needZoomToFit();
	LDObject* objectAtCursor() const;
	QSet<LDObject*> pick(const QRect& range);
	LDObject* pick(int mouseX, int mouseY);
	void resetAllAngles();
	void resetAngles();
	void setBackground();
	void setCamera(Camera cam);
	void setPicking(bool a);
	QPen textPen() const;

	static const QPen thinBorderPen;
	static const GLRotationMatrix topCameraMatrix;
	static const GLRotationMatrix frontCameraMatrix;
	static const GLRotationMatrix leftCameraMatrix;
	static const GLRotationMatrix bottomCameraMatrix;
	static const GLRotationMatrix backCameraMatrix;
	static const GLRotationMatrix rightCameraMatrix;
	static const GLRotationMatrix ldrawToGLAdapterMatrix;

signals:
	void objectHighlightingChanged(LDObject* object);

protected:
	void initializeGL();
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
	void leaveEvent(QEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* ev);
	void mouseReleaseEvent(QMouseEvent* ev);
	void paintEvent(QPaintEvent*);
	void resizeGL(int w, int h);
	void wheelEvent(QWheelEvent* ev);

	virtual void overpaint(QPainter& painter);
	virtual bool freeCameraAllowed() const;
	bool isDrawingSelectionScene() const;
	Qt::MouseButtons lastButtons() const;
	double panning (Axis ax) const;
	const QGenericMatrix<4, 4, GLfloat>& rotationMatrix() const;
	double zoom();

	template<typename... Args>
	QString format (QString fmtstr, Args... args)
	{
		return ::format (fmtstr, args...);
	}

private:
	const Model* const m_model;
	GLCompiler* m_compiler;
	LDObject* m_objectAtCursor = nullptr;
	CameraIcon m_cameraIcons[7];
	QTimer* m_toolTipTimer;
	Qt::MouseButtons m_lastButtons;
	Qt::KeyboardModifiers m_currentKeyboardModifiers;
	QGenericMatrix<4, 4, GLfloat> m_rotationMatrix;
	GLCamera m_cameras[7];
	bool m_useDarkBackground = false;
	bool m_takingScreenCapture = false;
	bool m_panning = false;
	bool m_initialized = false;
	bool m_isDrawingSelectionScene = false;
	bool m_isCameraMoving = false;
	bool m_needZoomToFit = true;
	bool m_axesInitialized = false;
	QPoint m_mousePosition;
	QPoint m_globalpos;
	QPointF m_mousePositionF;
	Camera m_camera;
	GLuint m_axeslist;
	int m_totalMouseMove;
	QColor m_backgroundColor;
	GLuint m_axesVbo;
	GLuint m_axesColorVbo;

	void calcCameraIcons();
	void drawVbos (VboClass surface, VboSubclass colors);
	void freeAxes();
	void zoomToFit();
	void zoomAllToFit();
	Q_SLOT void removeObject(LDObject* object);

private slots:
	void showCameraIconTooltip();
	void initializeAxes();
	void initializeLighting();
};

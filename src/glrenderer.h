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

#pragma once
#include <QGLWidget>
#include "main.h"
#include "model.h"
#include "glShared.h"
#include "glcamera.h"
#include "hierarchyelement.h"

namespace gl
{
	enum CameraType
	{
		TopCamera,
		FrontCamera,
		LeftCamera,
		BottomCamera,
		BackCamera,
		RightCamera,
		FreeCamera,
		_End
	};

	struct CameraIcon
	{
		QPixmap image;
		QRect sourceRect;
		QRect targetRect;
		QRect hitRect;
		CameraType camera;
	};

	class Renderer;
	class Compiler;

	static const QPen thinBorderPen {QColor {0, 0, 0, 208}, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin};

	// Transformation matrices for the fixed cameras.
	static const QMatrix4x4 topCameraMatrix = {};
	static const QMatrix4x4 frontCameraMatrix = {1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1};
	static const QMatrix4x4 leftCameraMatrix = {0, -1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0, 0, 0, 1};
	static const QMatrix4x4 bottomCameraMatrix = {1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1};
	static const QMatrix4x4 backCameraMatrix = {-1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1};
	static const QMatrix4x4 rightCameraMatrix = {0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1};

	// Conversion matrix from LDraw to OpenGL coordinates.
	static const QMatrix4x4 ldrawToGLAdapterMatrix = {1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1};

	enum { BlackRgb = 0xff000000 };
	static constexpr GLfloat near = 1.0f;
	static constexpr GLfloat far = 10000.0f;
}

MAKE_ITERABLE_ENUM(gl::CameraType)

// The main renderer object, draws the brick on the screen, manages the camera and selection picking.
class gl::Renderer : public QGLWidget, protected QOpenGLFunctions, public HierarchyElement
{
	Q_OBJECT

public:
	Renderer(const Model* model, QWidget* parent = nullptr);
	~Renderer();

	gl::CameraType camera() const;
	GLCamera& currentCamera();
	const GLCamera& currentCamera() const;
	Q_SLOT void fullUpdate();
	Qt::KeyboardModifiers keyboardModifiers() const;
	const Model* model() const;
	QPoint const& mousePosition() const;
	QPointF const& mousePositionF() const;
	QPersistentModelIndex objectAtCursor() const;
	QItemSelection pick(const QRect& range);
	QModelIndex pick(int mouseX, int mouseY);
	void resetAllAngles();
	void resetAngles();
	QImage screenCapture();
	void setBackground();
	void setCamera(gl::CameraType cam);
	QPen textPen() const;
	QItemSelectionModel* selectionModel() const;
	void setSelectionModel(QItemSelectionModel* selectionModel);

signals:
	void objectHighlightingChanged(const QModelIndex& oldIndex, const QModelIndex& newIndex);

protected:
	void initializeGL();
	virtual void drawFixedCameraBackdrop();
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
	void leaveEvent(QEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* ev);
	void mouseReleaseEvent(QMouseEvent* ev);
	void paintEvent(QPaintEvent*);
	void resizeGL(int w, int h);
	void wheelEvent(QWheelEvent* ev);

	QColor backgroundColor() const;
	virtual bool freeCameraAllowed() const;
	bool isDrawingSelectionScene() const;
	Qt::MouseButtons lastButtons() const;
	bool mouseHasMoved() const;
	virtual void overpaint(QPainter& painter);
	double panning (Axis ax) const;
	double zoom();

	template<typename... Args>
	QString format (QString fmtstr, Args... args)
	{
		return ::format (fmtstr, args...);
	}

private:
	const Model* const m_model;
	gl::Compiler* m_compiler;
	QPersistentModelIndex m_objectAtCursor;
	gl::CameraIcon m_cameraIcons[7];
	QTimer* m_toolTipTimer;
	Qt::MouseButtons m_lastButtons;
	Qt::KeyboardModifiers m_currentKeyboardModifiers;
	QQuaternion m_rotation;
	GLCamera m_cameras[7];
	bool m_useDarkBackground = false;
	bool m_panning = false;
	bool m_initialized = false;
	bool m_isDrawingSelectionScene = false;
	bool m_isCameraMoving = false;
	bool m_needZoomToFit = true;
	bool m_axesInitialized = false;
	QPoint m_mousePosition;
	QPoint m_globalpos;
	QPointF m_mousePositionF;
	gl::CameraType m_camera;
	GLuint m_axeslist;
	int m_totalMouseMove;
	QColor m_backgroundColor;
	GLuint m_axesVbo;
	GLuint m_axesColorVbo;

	void calcCameraIcons();
	void drawGLScene();
	void drawVbos(VboClass surface, VboSubclass colors);
	void freeAxes();
	void highlightCursorObject();
	void initializeAxes();
	void initializeLighting();
	void initGLData();
	void needZoomToFit();
	void setPicking(bool picking);
	Q_SLOT void showCameraIconTooltip();
	void zoomToFit();
	void zoomAllToFit();
};

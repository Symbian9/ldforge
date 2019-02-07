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

// The main renderer object, draws the brick on the screen, manages the camera and selection picking.
class gl::Renderer : public QGLWidget, protected QOpenGLFunctions, public HierarchyElement
{
	Q_OBJECT

public:
	Renderer(const Model* model, gl::CameraType cameraType, QWidget* parent = nullptr);
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
	void resetAngles();
	QImage screenCapture();
	void setBackground();
	QPen textPen() const;
	QItemSelectionModel* selectionModel() const;
	void setSelectionModel(QItemSelectionModel* selectionModel);

signals:
	void closed();
	void objectHighlightingChanged(const QModelIndex& oldIndex, const QModelIndex& newIndex);

protected:
	void closeEvent(QCloseEvent* event) override;
	void initializeGL() override;
	virtual void drawFixedCameraBackdrop();
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent* ev) override;
	void paintEvent(QPaintEvent*) override;
	void resizeGL(int w, int h) override;
	void wheelEvent(QWheelEvent* ev) override;

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
	gl::CameraType const m_camera;
	gl::Compiler* m_compiler;
	QPersistentModelIndex m_objectAtCursor;
	QTimer* m_toolTipTimer;
	Qt::MouseButtons m_lastButtons;
	Qt::KeyboardModifiers m_currentKeyboardModifiers;
	QQuaternion m_rotation;
	GLCamera m_cameraInfo;
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
	void zoomToFit();
	void zoomAllToFit();
};

/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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
#include "macros.h"
#include "ldObject.h"
#include "ldDocument.h"
#include "glShared.h"
#include "editmodes/abstractEditMode.h"

class GLCompiler;
class MessageManager;
class QDialogButtonBox;
class RadioGroup;
class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;
class QTimer;
class MagicWandMode;

struct CameraInfo
{
	int glrotate[3];
	Axis localX;
	Axis localY;
	bool negatedX;
	bool negatedY;
	bool negatedDepth; // is greater depth value closer to camera?
};

//
// Meta for overlays
//
struct LDGLOverlay
{
	LDGLOverlay();
	~LDGLOverlay();

	Vertex			v0,
					v1;
	int				offsetX,
					offsetY;
	double			width,
					height;
	QString			fileName;
	QImage*			image;
	bool			invalid;
};

//
// Document-specific data
//
struct LDGLData
{
	double			rotationX;
	double			rotationY;
	double			rotationZ;
	double			panX[7];
	double			panY[7];
	double			zoom[7];
	double			depthValues[6];
	LDGLOverlay		overlays[6];
	bool			init;
	bool			needZoomToFit;

	LDGLData() :
		rotationX (0.0),
		rotationY (0.0),
		rotationZ (0.0),
		init (false),
		needZoomToFit (true)
	{
		for (int i = 0; i < 7; ++i)
		{
			if (i < 6)
			{
				overlays[i].image = nullptr;
				overlays[i].invalid = false;
				depthValues[i] = 0.0f;
			}

			zoom[i] = 30.0;
			panX[i] = 0.0;
			panY[i] = 0.0;
		}
	}
};

enum Camera
{
	TopCamera,
	FrontCamera,
	LeftCamera,
	BottomCamera,
	BackCamera,
	RightCamera,
	FreeCamera,
};

MAKE_ITERABLE_ENUM(Camera, TopCamera, FreeCamera)

struct CameraIcon
{
	QPixmap image;
	QRect sourceRect;
	QRect targetRect;
	QRect hitRect;
	Camera camera;
};

// The main renderer object, draws the brick on the screen, manages the camera and selection picking.
class GLRenderer : public QGLWidget, protected QOpenGLFunctions, public HierarchyElement
{
	Q_OBJECT

public:
	GLRenderer (QWidget* parent = nullptr);
	~GLRenderer();

	Camera camera() const;
	const CameraInfo& cameraInfo (Camera camera) const;
	QString cameraName (Camera camera) const;
	QByteArray capturePixels();
	void clearOverlay();
	void compileObject (LDObject* obj);
	GLCompiler* compiler() const;
	Vertex convert2dTo3d (const QPoint& pos2d, bool snap) const;
	QPoint convert3dTo2d (const Vertex& pos3d);
	QString currentCameraName() const;
	EditModeType currentEditModeType() const;
	int depthNegateFactor() const;
	LDDocument* document() const;
	void drawBlip (QPainter& painter, QPointF pos, QColor color = QColor (64, 192, 0)) const;
	void drawBlipCoordinates (QPainter& painter, const Vertex& pos3d);
	void drawBlipCoordinates (QPainter& painter, const Vertex& pos3d, QPointF pos);
	void drawGLScene();
	void forgetObject (LDObject* obj);
	Axis getCameraAxis (bool y, Camera camid = (Camera) -1);
	double getDepthValue() const;
	LDGLOverlay& getOverlay (int newcam);
	void getRelativeAxes (Axis& relX, Axis& relY) const;
	Axis getRelativeZ() const;
	void hardRefresh();
	void highlightCursorObject();
	void initGLData();
	void initOverlaysFromObjects();
	bool isDrawOnly() const;
	bool isPicking() const;
	Qt::KeyboardModifiers keyboardModifiers() const;
	QPen linePen() const;
	void makeCurrent();
	MessageManager* messageLog() const;
	bool mouseHasMoved() const;
	QPoint const& mousePosition() const;
	QPointF const& mousePositionF() const;
	void needZoomToFit();
	LDObject* objectAtCursor() const;
	void pick (int mouseX, int mouseY, bool additive);
	void pick (QRect const& range, bool additive);
	LDObject* pickOneObject (int mouseX, int mouseY);
	Vertex const& position3D() const;
	void refresh();
	void resetAllAngles();
	void resetAngles();
	void setBackground();
	void setCamera (const Camera cam);
	void setDepthValue (double depth);
	void setDocument (LDDocument* document);
	void setDrawOnly (bool value);
	void setEditMode (EditModeType type);
	void setPicking (bool a);
	bool setupOverlay (Camera camera, QString fileName, int x, int y, int w, int h);
	QPen textPen() const;
	void updateOverlayObjects();
	void zoomNotch (bool inward);

protected:
	void contextMenuEvent (QContextMenuEvent* ev);
	void dragEnterEvent (QDragEnterEvent* ev);
	void dropEvent (QDropEvent* ev);
	void initializeGL();
	void keyPressEvent (QKeyEvent* ev);
	void keyReleaseEvent (QKeyEvent* ev);
	void leaveEvent (QEvent* ev);
	void mouseDoubleClickEvent (QMouseEvent* ev);
	void mousePressEvent (QMouseEvent* ev);
	void mouseMoveEvent (QMouseEvent* ev);
	void mouseReleaseEvent (QMouseEvent* ev);
	void paintEvent (QPaintEvent* ev);
	void resizeGL (int w, int h);
	void wheelEvent (QWheelEvent* ev);

private:
	MessageManager* m_messageLog;
	LDDocument* m_document;
	GLCompiler* m_compiler;
	LDObject* m_objectAtCursor;

	CameraIcon m_cameraIcons[7];
	QTimer* m_toolTipTimer;
	Qt::MouseButtons m_lastButtons;
	Qt::KeyboardModifiers m_currentKeyboardModifiers;
	Vertex m_position3D;
	double m_virtualWidth;
	double m_virtualHeight;
	bool m_useDarkBackground;
	bool m_drawToolTip;
	bool m_takingScreenCapture;
	bool m_panning;
	bool m_initialized;
	bool m_isDrawOnly;
	bool m_isPicking;
	bool m_isCameraMoving;
	QPoint m_mousePosition;
	QPoint m_globalpos;
	QPointF m_mousePositionF;
	QPen m_thinBorderPen;
	Camera m_camera;
	Camera m_toolTipCamera;
	GLuint m_axeslist;
	int m_width;
	int m_height;
	int m_totalMouseMove;
	QColor m_backgroundColor;
	AbstractEditMode* m_currentEditMode;
	GLuint m_axesVbo;
	GLuint m_axesColorVbo;

	void calcCameraIcons();
	void clampAngle (double& angle) const;
	LDGLData& currentDocumentData() const;
	void drawVbos (SurfaceVboType surface, ComplementVboType colors, GLenum type);
	LDOverlay* findOverlayObject (Camera cam);
	double& panning (Axis ax);
	double panning (Axis ax) const;
	double& rotation (Axis ax);
	double& zoom();
	void zoomToFit();
	void zoomAllToFit();

	template<typename... Args>
	QString format (QString fmtstr, Args... args)
	{
		return ::format (fmtstr, args...);
	}

private slots:
	void	slot_toolTipTimer();
	void	initializeAxes();
};

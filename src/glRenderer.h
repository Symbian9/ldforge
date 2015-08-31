/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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

//
// Meta for overlays
//
struct LDGLOverlay
{
	Vertex			v0,
					v1;
	int				ox,
					oy;
	double			lw,
					lh;
	QString			fname;
	QImage*			img;
	bool			invalid;
};

struct LDFixedCamera
{
	const char		glrotate[3];
	const Axis		axisX,
					axisY;
	const bool		negX,
					negY,
					negatedDepth; // is greater depth value closer to camera?
};

//
// Document-specific data
//
struct LDGLData
{
	double			rotX,
					rotY,
					rotZ,
					panX[7],
					panY[7],
					zoom[7];
	double			depthValues[6];
	LDGLOverlay		overlays[6];
	bool			init;
	bool			needZoomToFit;

	LDGLData() :
		rotX (0.0),
		rotY (0.0),
		rotZ (0.0),
		init (false),
		needZoomToFit (true)
	{
		for (int i = 0; i < 7; ++i)
		{
			if (i < 6)
			{
				overlays[i].img = null;
				overlays[i].invalid = false;
				depthValues[i] = 0.0f;
			}

			zoom[i] = 30.0;
			panX[i] = 0.0;
			panY[i] = 0.0;
		}
	}
};

enum ECamera
{
	ETopCamera,
	EFrontCamera,
	ELeftCamera,
	EBottomCamera,
	EBackCamera,
	ERightCamera,
	EFreeCamera,

	ENumCameras,
	EFirstCamera = ETopCamera
};

MAKE_ITERABLE_ENUM (ECamera)

//
// CameraIcon::img is a heap-allocated QPixmap because otherwise it gets
// initialized before program gets to main() and constructs a QApplication
// and Qt doesn't like that.
//
struct CameraIcon
{
	QPixmap*		img;
	QRect			srcRect,
					destRect,
					selRect;
	ECamera			cam;
};

//
// The main renderer object, draws the brick on the screen, manages the camera
// and selection picking. The instance of GLRenderer is accessible as
// g_win->R()
//
class GLRenderer : public QGLWidget, protected QOpenGLFunctions, public HierarchyElement
{
public:
	Q_OBJECT
	PROPERTY (public,	bool,				isDrawOnly,		setDrawOnly,		STOCK_WRITE)
	PROPERTY (public,	MessageManager*,	messageLog, 	setMessageLog,		STOCK_WRITE)
	PROPERTY (private,	bool,				isPicking,		setPicking,			CUSTOM_WRITE)
	PROPERTY (public,	LDDocument*,		document,		setDocument,		CUSTOM_WRITE)
	PROPERTY (public,	GLCompiler*,		compiler,		setCompiler,		STOCK_WRITE)
	PROPERTY (public,	LDObject*,	objectAtCursor,	setObjectAtCursor,	STOCK_WRITE)
	PROPERTY (private,	bool,				isCameraMoving,	setCameraMoving,	STOCK_WRITE)

public:
	GLRenderer (QWidget* parent = null);
	~GLRenderer();

	inline ECamera			camera() const;
	void					clearOverlay();
	void					compileObject (LDObject* obj);
	Vertex					coordconv2_3 (const QPoint& pos2d, bool snap) const;
	QPoint					coordconv3_2 (const Vertex& pos3d);
	EditModeType			currentEditModeType() const;
	int						depthNegateFactor() const;
	void					drawBlip (QPainter& paint, QPointF pos) const;
	void					drawGLScene();
	void					forgetObject (LDObject* obj);
	Axis					getCameraAxis (bool y, ECamera camid = (ECamera) -1);
	const char*				getCameraName() const;
	double					getDepthValue() const;
	LDFixedCamera const&	getFixedCamera (ECamera cam) const;
	void					getRelativeAxes (Axis& relX, Axis& relY) const;
	Axis					getRelativeZ() const;
	LDGLOverlay&			getOverlay (int newcam);
	uchar*					getScreencap (int& w, int& h);
	Qt::KeyboardModifiers	keyboardModifiers() const;
	void					hardRefresh();
	void					highlightCursorObject();
	void					initGLData();
	void					initOverlaysFromObjects();
	QPen					linePen() const;
	bool					mouseHasMoved() const;
	QPoint const&			mousePosition() const;
	QPointF const&			mousePositionF() const;
	void					needZoomToFit();
	void					pick (int mouseX, int mouseY, bool additive);
	void					pick (QRect const& range, bool additive);
	LDObject*				pickOneObject (int mouseX, int mouseY);
	Vertex const&			position3D() const;
	void					refresh();
	void					resetAngles();
	void					resetAllAngles();
	void					setBackground();
	void					setCamera (const ECamera cam);
	void					setDepthValue (double depth);
	void					setEditMode (EditModeType type);
	bool					setupOverlay (ECamera cam, QString file, int x, int y, int w, int h);
	QPen					textPen() const;
	void					updateOverlayObjects();
	void					zoomNotch (bool inward);

	QColor					getMainColor();

protected:
	void					contextMenuEvent (QContextMenuEvent* ev);
	void					dragEnterEvent (QDragEnterEvent* ev);
	void					dropEvent (QDropEvent* ev);
	void					initializeGL();
	void					keyPressEvent (QKeyEvent* ev);
	void					keyReleaseEvent (QKeyEvent* ev);
	void					leaveEvent (QEvent* ev);
	void					mouseDoubleClickEvent (QMouseEvent* ev);
	void					mousePressEvent (QMouseEvent* ev);
	void					mouseMoveEvent (QMouseEvent* ev);
	void					mouseReleaseEvent (QMouseEvent* ev);
	void					paintEvent (QPaintEvent* ev);
	void					resizeGL (int w, int h);
	void					wheelEvent (QWheelEvent* ev);

private:
	CameraIcon				m_cameraIcons[7];
	QTimer*					m_toolTipTimer;
	Qt::MouseButtons		m_lastButtons;
	Qt::KeyboardModifiers	m_keymods;
	Vertex					m_position3D;
	double					m_virtWidth,
							m_virtHeight;
	bool					m_darkbg,
							m_drawToolTip,
							m_screencap,
							m_panning;
	QPoint					m_mousePosition,
							m_globalpos;
	QPointF					m_mousePositionF;
	QPen					m_thinBorderPen;
	ECamera					m_camera,
							m_toolTipCamera;
	GLuint					m_axeslist;
	int						m_width,
							m_height,
							m_totalmove;
	QColor					m_bgcolor;
	AbstractEditMode*		m_editmode;
	GLuint					m_axesVBO;
	GLuint					m_axesColorVBO;

	void					calcCameraIcons();
	void					clampAngle (double& angle) const;
	inline LDGLData&		currentDocumentData() const;
	void					drawVBOs (EVBOSurface surface, EVBOComplement colors, GLenum type);
	void					doMakeCurrent();
	LDOverlay*			findOverlayObject (ECamera cam);
	inline double&			pan (Axis ax);
	inline const double&	pan (Axis ax) const;
	inline double&			rot (Axis ax);
	inline double&			zoom();
	void					zoomToFit();
	void					zoomAllToFit();

	template<typename... Args>
	inline QString format (QString fmtstr, Args... args)
	{
		return ::format (fmtstr, args...);
	}

private slots:
	void	slot_toolTipTimer();
	void	initializeAxes();
};

inline ECamera GLRenderer::camera() const
{
	return m_camera;
}

inline LDGLData& GLRenderer::currentDocumentData() const
{
	return *document()->getGLData();
}

inline double& GLRenderer::rot (Axis ax)
{
	return
		(ax == X) ? currentDocumentData().rotX :
		(ax == Y) ? currentDocumentData().rotY :
					currentDocumentData().rotZ;
}

inline double& GLRenderer::pan (Axis ax)
{
	return (ax == X) ? currentDocumentData().panX[camera()] :
		currentDocumentData().panY[camera()];
}

inline double const& GLRenderer::pan (Axis ax) const
{
	return (ax == X) ? currentDocumentData().panX[camera()] :
		currentDocumentData().panY[camera()];
}

inline double& GLRenderer::zoom()
{
	return currentDocumentData().zoom[camera()];
}

LDFixedCamera const& GetFixedCamera (ECamera cam);

extern const char* g_CameraNames[7];

/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#include <QtGui>
#include <QGLWidget>
#include <GL/glu.h>
#include "common.h"
#include "config.h"
#include "file.h"
#include "gldraw.h"
#include "bbox.h"
#include "colors.h"
#include "gui.h"
#include "misc.h"
#include "history.h"

static double g_objOffset[3];

static const struct staticCameraMeta {
	const char glrotate[3];
	const Axis axisX, axisY;
	const bool negX, negY;
} g_staticCameras[6] = {
	{ { 1, 0, 0 }, X, Z, false, false },
	{ { 0, 0, 0 }, X, Y, false, true },
	{ { 0, 1, 0 }, Z, Y, true, true },
	{ { -1, 0, 0 }, X, Z, false, true },
	{ { 0, 0, 0 }, X, Y, true, true },
	{ { 0, -1, 0 }, Z, Y, false, true },
};

cfg (str, gl_bgcolor, "#CCCCD9");
cfg (str, gl_maincolor, "#707078");
cfg (float, gl_maincolor_alpha, 1.0);
cfg (int, gl_linethickness, 2);
cfg (bool, gl_colorbfc, true);
cfg (int, gl_camera, GLRenderer::Free);
cfg (bool, gl_blackedges, true);
cfg (bool, gl_axes, false);

// CameraIcon::img is a heap-allocated QPixmap because otherwise it gets
// initialized before program gets to main() and constructs a QApplication
// and Qt doesn't like that.
struct CameraIcon {
	QPixmap* img;
	QRect srcRect, destRect, selRect;
	GLRenderer::Camera cam;
} g_CameraIcons[7];

const char* g_CameraNames[7] = { "Top", "Front", "Left", "Bottom", "Back", "Right", "Free" };

const GLRenderer::Camera g_Cameras[7] = {
	GLRenderer::Top,
	GLRenderer::Front,
	GLRenderer::Left,
	GLRenderer::Bottom,
	GLRenderer::Back,
	GLRenderer::Right,
	GLRenderer::Free
};

const struct GLAxis {
	const QColor col;
	const vertex vert;
} g_GLAxes[3] = {
	{ QColor (255, 0, 0), vertex (10000, 0, 0) },
	{ QColor (128, 192, 0), vertex (0, 10000, 0) },
	{ QColor (0, 160, 192), vertex (0, 0, 10000) },
};
	
// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
GLRenderer::GLRenderer (QWidget* parent) : QGLWidget (parent) {
	resetAngles ();
	m_picking = m_rangepick = false;
	m_camera = (GLRenderer::Camera) gl_camera.value;
	m_drawToolTip = false;
	m_planeDraw = false;
	
	m_toolTipTimer = new QTimer (this);
	m_toolTipTimer->setSingleShot (true);
	connect (m_toolTipTimer, SIGNAL (timeout ()), this, SLOT (slot_toolTipTimer ()));
	
	m_thickBorderPen = QPen (QColor (0, 0, 0, 208), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	m_thinBorderPen = m_thickBorderPen;
	m_thinBorderPen.setWidth (1);
	
	// Init camera icons
	for (const GLRenderer::Camera cam : g_Cameras) {
		str iconname;
		iconname.format ("camera-%s", str (g_CameraNames[cam]).lower ().c ());
		
		CameraIcon* info = &g_CameraIcons[cam];
		info->img = new QPixmap (getIcon (iconname));
		info->cam = cam;
	}
	
	calcCameraIcons ();
}

// =============================================================================
GLRenderer::~GLRenderer() {
	for (CameraIcon& info : g_CameraIcons)
		delete info.img;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::calcCameraIcons () {
	ushort i = 0;
	
	for (CameraIcon& info : g_CameraIcons) {
		const long x1 = (m_width - (info.cam != Free ? 48 : 16)) + ((i % 3) * 16) - 1,
			y1 = ((i / 3) * 16) + 1;
		
		info.srcRect = QRect (0, 0, 16, 16);
		info.destRect = QRect (x1, y1, 16, 16);
		info.selRect = QRect (info.destRect.x (), info.destRect.y (),
			info.destRect.width () + 1, info.destRect.height () + 1);
		++i;
	}
}

// =============================================================================
void GLRenderer::resetAngles () {
	m_rotX = 30.0f;
	m_rotY = 325.f;
	m_panX = m_panY = m_rotZ = 0.0f;
	
	// Set the default zoom based on the bounding box
	m_zoom = g_BBox.size () * 6;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::initializeGL () {
	setBackground ();
	
	glLineWidth (gl_linethickness);
	
	setAutoFillBackground (false);
	setMouseTracking (true);
	setFocusPolicy (Qt::WheelFocus);
	compileAllObjects ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
QColor GLRenderer::getMainColor () {
	QColor col (gl_maincolor.value.chars());
	
	if (!col.isValid ())
		return QColor (0, 0, 0);
	
	col.setAlpha (gl_maincolor_alpha * 255.f);
	return col;
}

// -----------------------------------------------------------------------------
void GLRenderer::setBackground () {
	QColor col (gl_bgcolor.value.chars());
	
	if (!col.isValid ())
		return;
	
	m_darkbg = luma (col) < 80;
	
	col.setAlpha (255);
	qglClearColor (col);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static vector<short> g_warnedColors;
void GLRenderer::setObjectColor (LDObject* obj, const ListType list) {
	QColor qcol;
	
	if (!obj->isColored ())
		return;
	
	if (list == GL::PickList) {
		// Make the color by the object's index color if we're picking, so we can
		// make the index from the color we get from the picking results. Be sure
		// to use the top level parent's index since inlinees don't have an index.
		long i = obj->topLevelParent ()->getIndex (g_curfile);
		
		// We should have the index now.
		assert (i != -1);
		
		// Calculate a color based from this index. This method caters for
		// 16777216 objects. I don't think that'll be exceeded anytime soon. :)
		// ATM biggest is 53588.dat with 12600 lines.
		double r = (i / (256 * 256)) % 256,
			g = (i / 256) % 256,
			b = i % 256;
		
		qglColor (QColor (r, g, b, 255));
		return;
	}
	
	if ((list == BFCFrontList || list == BFCBackList) &&
		obj->getType () != LDObject::Line &&
		obj->getType () != LDObject::CondLine)
	{
		if (list == GL::BFCFrontList)
			qcol = QColor (80, 192, 0);
		else
			qcol = QColor (224, 0, 0);
	} else {
		if (obj->color == maincolor)
			qcol = getMainColor ();
		else {
			color* col = getColor (obj->color);
			qcol = col->faceColor;
		}
		
		if (obj->color == edgecolor) {
			qcol = Qt::black;
			color* col;
			
			if (!gl_blackedges && obj->parent != null && (col = getColor (obj->parent->color)) != null)
				qcol = col->edgeColor;
		}
		
		if (qcol.isValid () == false) {
			// The color was unknown. Use main color to make the object at least
			// not appear pitch-black.
			if (obj->color != edgecolor)
				qcol = getMainColor ();
			
			// Warn about the unknown colors, but only once.
			for (short i : g_warnedColors)
				if (obj->color == i)
					return;
			
			printf ("%s: Unknown color %d!\n", __func__, obj->color);
			g_warnedColors.push_back (obj->color);
			return;
		}
	}
	
	long r = qcol.red (),
		g = qcol.green (),
		b = qcol.blue (),
		a = qcol.alpha ();
	
	if (obj->topLevelParent ()->selected ()) {
		// Brighten it up for the select list.
		const uchar add = 51;
		
		r = min (r + add, 255l);
		g = min (g + add, 255l);
		b = min (b + add, 255l);
	}
	
	glColor4f (
		((double) r) / 255.0f,
		((double) g) / 255.0f,
		((double) b) / 255.0f,
		((double) a) / 255.0f);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::refresh () {
	update ();
	swapBuffers ();
}

// =============================================================================
void GLRenderer::hardRefresh () {
	compileAllObjects ();
	refresh ();
	
	glLineWidth (gl_linethickness);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::resizeGL (int w, int h) {
	m_width = w;
	m_height = h;
	
	calcCameraIcons ();
	
	glViewport (0, 0, w, h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (45.0f, (double)w / (double)h, 1.0f, 10000.0f);
	glMatrixMode (GL_MODELVIEW);
}

void GLRenderer::drawGLScene () const {
	if (g_curfile == null)
		return;
	
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable (GL_DEPTH_TEST);
	
	if (m_camera != GLRenderer::Free) {
		glMatrixMode (GL_PROJECTION);
		glPushMatrix ();
		
		glLoadIdentity ();
		glOrtho (-m_virtWidth, m_virtWidth, -m_virtHeight, m_virtHeight, -100.0f, 100.0f);
		glTranslatef (m_panX, m_panY, 0.0f);
		glRotatef (90.0f, g_staticCameras[m_camera].glrotate[0],
			g_staticCameras[m_camera].glrotate[1],
			g_staticCameras[m_camera].glrotate[2]);
		
		// Back camera needs to be handled differently
		if (m_camera == GLRenderer::Back) {
			glRotatef (180.0f, 1.0f, 0.0f, 0.0f);
			glRotatef (180.0f, 0.0f, 0.0f, 1.0f);
		}
	} else {
		glMatrixMode (GL_MODELVIEW);
		glPushMatrix ();
		glLoadIdentity ();
		
		glTranslatef (0.0f, 0.0f, -2.0f);
		glTranslatef (m_panX, m_panY, -m_zoom);
		glRotatef (m_rotX, 1.0f, 0.0f, 0.0f);
		glRotatef (m_rotY, 0.0f, 1.0f, 0.0f);
		glRotatef (m_rotZ, 0.0f, 0.0f, 1.0f);
	}
	
	if (gl_colorbfc && !m_picking) {
		glEnable (GL_CULL_FACE);
		
		for (LDObject* obj : g_curfile->m_objs) {
			if (obj->hidden ())
				continue;
			
			glCullFace (GL_BACK);
			glCallList (obj->glLists[BFCFrontList]);
			
			glCullFace (GL_FRONT);
			glCallList (obj->glLists[BFCBackList]);
		}
		
		glDisable (GL_CULL_FACE);
	} else {
		for (LDObject* obj : g_curfile->m_objs) {
			if (obj->hidden ())
				continue;
			
			glCallList (obj->glLists[(m_picking) ? PickList : NormalList]);
		}
	}
	
	if (gl_axes && !m_picking)
		glCallList (m_axeslist);
	
	glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);
}

// =============================================================================
vertex GLRenderer::coord_2to3 (const QPoint& pos2d, const bool snap) const {
	vertex pos3d;
	const staticCameraMeta* cam = &g_staticCameras[m_camera];
	const Axis axisX = cam->axisX;
	const Axis axisY = cam->axisY;
	const short negXFac = cam->negX ? -1 : 1,
		negYFac = cam->negY ? -1 : 1;
	
	// Calculate cx and cy - these are the LDraw unit coords the cursor is at.
	double cx = (-m_virtWidth + ((2 * pos2d.x () * m_virtWidth) / m_width) - m_panX) - (negXFac * g_objOffset[axisX]);
	double cy = (m_virtHeight - ((2 * pos2d.y () * m_virtHeight) / m_height) - m_panY) - (negYFac * g_objOffset[axisY]);
	
	if (snap) {
		cx = Grid::snap (cx, (Grid::Config) axisX);
		cy = Grid::snap (cy, (Grid::Config) axisY);
	}
	
	cx *= negXFac;
	cy *= negYFac;
	
	pos3d = g_origin;
	pos3d[axisX] = cx;
	pos3d[axisY] = cy;
	return pos3d;
}

// =============================================================================
QPoint GLRenderer::coord_3to2 (const vertex& pos3d) const {
	/*
	cx = (-m_virtWidth + ((2 * pos2d.x () * m_virtWidth) / m_width) - m_panX) - (negXFac * g_objOffset[axisX]);
	
	                                                 cx = (-vw + ((2 * x * vw) / w) - panx) - (neg * ofs)
	                                  cx + (neg * ofs) = (-vw + ((2 * x * vw) / w) - panx)
	                                  cx + (neg * ofs) = ((2 * x * vw) / w) - vw - panx
	                    (cx + (neg * ofs)) + vw + panx = (2 * x * vw) / w
	              ((cx + (neg * ofs)) + vw + panx) * w = 2 * vw * x
	
	x = (((cx + (neg * ofs)) + vw + panx) * w) / (2 * vw)
	*/
	
	QPoint pos2d;
	const staticCameraMeta* cam = &g_staticCameras[m_camera];
	const Axis axisX = cam->axisX;
	const Axis axisY = cam->axisY;
	const short negXFac = cam->negX ? -1 : 1,
		negYFac = cam->negY ? -1 : 1;
	
	short x1 = (((pos3d[axisX] + (negXFac * g_objOffset[axisX])) +
		m_virtWidth + m_panX) * m_width) / (2 * m_virtWidth);
	short y1 = -(((pos3d[axisY] + (negYFac * g_objOffset[axisY])) -
		m_virtHeight + m_panY) * m_height) / (2 * m_virtHeight);
	
	x1 *= negXFac;
	y1 *= negYFac;
	
	pos2d = QPoint (x1, y1);
	return pos2d;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::paintEvent (QPaintEvent* ev) {
	Q_UNUSED (ev)
	m_virtWidth = m_zoom;
	m_virtHeight = (m_height * m_virtWidth) / m_width;
	
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1.0f, 1.0f);
	
	glEnable (GL_DEPTH_TEST);
	glShadeModel (GL_SMOOTH);
	glEnable (GL_MULTISAMPLE);
	
	glEnable (GL_LINE_SMOOTH);
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
	
	drawGLScene ();
	
	QPainter paint (this);
	QFontMetrics metrics = QFontMetrics (QFont ());
	paint.setRenderHint (QPainter::Antialiasing);
	
	m_hoverpos = g_origin;
	
	if (m_camera != Free) {
		// Calculate 3d position of the cursor
		m_hoverpos = coord_2to3 (m_pos, true);
		
		// Paint the coordinates onto the screen.
		str text;
		text.format ("X: %s, Y: %s, Z: %s", ftoa (m_hoverpos[X]).chars (),
			ftoa (m_hoverpos[Y]).chars (), ftoa (m_hoverpos[Z]).chars ());
		
		QFontMetrics metrics = QFontMetrics (font ());
		QRect textSize = metrics.boundingRect (0, 0, m_width, m_height, Qt::AlignCenter, text);
		
		paint.drawText (m_width - textSize.width (), m_height - 16, textSize.width (),
			textSize.height (), Qt::AlignCenter, text);
		
		// If we're plane drawing, draw the vertices onto the screen.
		if (m_planeDraw) {
			ushort numverts = m_planeDrawVerts.size () + 1;
			const short blipsize = 8;
			
			if (numverts > 0) {
				QPoint* poly = new QPoint[numverts];
				
				uchar i = 0;
				for (vertex& vert : m_planeDrawVerts) {
					poly[i] = coord_3to2 (vert);
					++i;
				}
				
				// Draw the cursor vertex as the last one in the list.
				if (numverts < 5)
					poly[i] = coord_3to2 (m_hoverpos);
				else
					numverts = 4;
				
				paint.setPen (m_thinBorderPen);
				paint.setBrush (QColor (128, 192, 0));
				
				// Draw vertex blips
				for (ushort i = 0; i < numverts; ++i) {
					QPoint& blip = poly[i];
					paint.drawEllipse (blip.x () - blipsize / 2, blip.y () - blipsize / 2,
						blipsize, blipsize);
				}
				
				paint.setPen (m_thickBorderPen);
				paint.setBrush (QColor (128, 192, 0, 128));
				paint.drawPolygon (poly, numverts);
				
				delete[] poly;
			}
		}
	}
	
	// Camera icons
	if (!m_picking && m_planeDraw == false) {
		// Draw a background for the selected camera
		paint.setPen (m_thinBorderPen);
		paint.setBrush (QBrush (QColor (0, 128, 160, 128)));
		paint.drawRect (g_CameraIcons[camera ()].selRect);
		
		// Draw the actual icons
		for (CameraIcon& info : g_CameraIcons)
			paint.drawPixmap (info.destRect, *info.img, info.srcRect);
		
		// Draw a label for the current camera in the top left corner
		{
			const ushort margin = 4;
			
			str label;
			label.format ("%s Camera", g_CameraNames[camera ()]);
			paint.setBrush (Qt::black);
			paint.drawText (QPoint (margin, margin + metrics.ascent ()), label);
		}
		
		// Tool tips
		if (m_drawToolTip) {
			if (g_CameraIcons[m_toolTipCamera].destRect.contains (m_pos) == false)
				m_drawToolTip = false;
			else {
				QPen bord = m_thinBorderPen;
				bord.setBrush (Qt::black);
				
				const ushort margin = 2;
				ushort x0 = m_pos.x (),
					y0 = m_pos.y ();
				
				str label;
				label.format ("%s Camera", g_CameraNames[m_toolTipCamera]);
				
				const ushort textWidth = metrics.width (label),
					textHeight = metrics.height (),
					fullWidth = textWidth + (2 * margin),
					fullHeight = textHeight + (2 * margin);
				
				QRect area (m_pos.x (), m_pos.y (), fullWidth, fullHeight);
				
				if (x0 + fullWidth > m_width)
					x0 -= fullWidth;
				
				if (y0 + fullHeight > m_height)
					y0 -= fullHeight;
				
				paint.setBrush (QColor (0, 128, 255, 208));
				paint.setPen (bord);
				paint.drawRect (x0, y0, fullWidth, fullHeight);
				
				paint.setBrush (Qt::black);
				paint.drawText (QPoint (x0 + margin, y0 + margin + metrics.ascent ()), label);
			}
		}
	}
	
	// If we're range-picking, draw a rectangle encompassing the selection area.
	if (m_rangepick && !m_picking) {
		const short x0 = m_rangeStart.x (),
			y0 = m_rangeStart.y (),
			x1 = m_pos.x (),
			y1 = m_pos.y ();
		
		QRect rect (x0, y0, x1 - x0, y1 - y0);
		QColor fillColor = (m_addpick ? "#80FF00" : "#00CCFF");
		fillColor.setAlphaF (0.2f);
		
		paint.setPen (m_thickBorderPen);
		paint.setBrush (QBrush (fillColor));
		paint.drawRect (rect);
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::compileAllObjects () {
	if (g_BBox.empty () == false) {
		g_objOffset[X] = -(g_BBox.v0 ()[X] + g_BBox.v1 ()[X]) / 2;
		g_objOffset[Y] = -(g_BBox.v0 ()[Y] + g_BBox.v1 ()[Y]) / 2;
		g_objOffset[Z] = -(g_BBox.v0 ()[Z] + g_BBox.v1 ()[Z]) / 2;
	} else {
		// use a default bbox if we need 
		g_objOffset[X] = g_objOffset[Y] = g_objOffset[Z] = 0;
	}
	
	if (!g_curfile) {
		printf ("renderer: no files loaded, cannot compile anything\n");
		return;
	}
	
	for (LDObject* obj : g_curfile->m_objs)
		compileObject (obj);
	
	// Compile axes
	m_axeslist = glGenLists (1);
	glNewList (m_axeslist, GL_COMPILE);
	glBegin (GL_LINES);
	for (const GLAxis& ax : g_GLAxes) {
		qglColor (ax.col);
		compileVertex (ax.vert);
		compileVertex (-ax.vert);
	}
	glEnd ();
	glEndList ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static bool g_glInvert = false;

void GLRenderer::compileSubObject (LDObject* obj, const GLenum gltype) {
	glBegin (gltype);
	
	const short numverts = (obj->getType () != LDObject::CondLine) ? obj->vertices () : 2;
	
	if (g_glInvert == false)
		for (short i = 0; i < numverts; ++i)
			compileVertex (obj->coords[i]);
	else
		for (short i = numverts - 1; i >= 0; --i)
			compileVertex (obj->coords[i]);
	
	glEnd ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::compileList (LDObject* obj, const GLRenderer::ListType list) {
	setObjectColor (obj, list);
	
	switch (obj->getType ()) {
	case LDObject::Line:
		compileSubObject (obj, GL_LINES);
		break;
	
	case LDObject::CondLine:
		if (list != GL::PickList) {
			glLineStipple (1, 0x6666);
			glEnable (GL_LINE_STIPPLE);
		}
		
		compileSubObject (obj, GL_LINES);
		
		glDisable (GL_LINE_STIPPLE);
		break;
	
	case LDObject::Triangle:
		compileSubObject (obj, GL_TRIANGLES);
		break;
	
	case LDObject::Quad:
		compileSubObject (obj, GL_QUADS);
		break;
	
	case LDObject::Subfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			vector<LDObject*> objs = ref->inlineContents (true, true);
			
			bool oldinvert = g_glInvert;
			
			if (ref->transform.determinant () < 0)
				g_glInvert = !g_glInvert;
			
			LDObject* prev = ref->prev ();
			if (prev->getType () == LDObject::BFC && static_cast<LDBFC*> (prev)->type == LDBFC::InvertNext)
				g_glInvert = !g_glInvert;
			
			for (LDObject* obj : objs) {
				compileList (obj, list);
				delete obj;
			}
			
			g_glInvert = oldinvert;
		}
		break;
	
	case LDObject::Radial:
		{
			LDRadial* rad = static_cast<LDRadial*> (obj);
			std::vector<LDObject*> objs = rad->decompose (true);
			
			bool oldinvert = g_glInvert;
			if (rad->transform.determinant () < 0)
				g_glInvert = !g_glInvert;
			
			LDObject* prev = rad->prev ();
			if (prev->getType () == LDObject::BFC && static_cast<LDBFC*> (prev)->type == LDBFC::InvertNext)
				g_glInvert = !g_glInvert;
			
			for (LDObject* obj : objs) {
				compileList (obj, list);
				delete obj;
			}
			
			g_glInvert = oldinvert;
		}
		break;
	
#if 0
	TODO: find a proper way to draw vertices without having them be affected by zoom.
	case LDObject::Vertex:
		{
			LDVertex* pVert = static_cast<LDVertex*> (obj);
			LDTriangle* pPoly;
			vertex* vPos = &(pVert->pos);
			const double fPolyScale = max (fZoom, 1.0);
			
#define BIPYRAMID_COORD(N) ((((i + N) % 4) >= 2 ? 1 : -1) * 0.3f * fPolyScale)
			
			for (int i = 0; i < 8; ++i) {
				pPoly = new LDTriangle;
				pPoly->coords[0] = {vPos->x, vPos->y + ((i >= 4 ? 1 : -1) * 0.4f * fPolyScale), vPos->z};
				pPoly->coords[1] = {
					vPos->x + BIPYRAMID_COORD (0),
					vPos->y,
					vPos->z + BIPYRAMID_COORD (1)
				};
				
				pPoly->coords[2] = {
					vPos->x + BIPYRAMID_COORD (1),
					vPos->y,
					vPos->z + BIPYRAMID_COORD (2)
				};
				
				pPoly->dColor = pVert->dColor;
				compileOneObject (pPoly, list);
				delete pPoly;
			}
		}
		break;
#endif // 0
	
	default:
		break;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::compileVertex (const vertex& vrt) {
	glVertex3d (
		(vrt[X] + g_objOffset[0]),
		-(vrt[Y] + g_objOffset[1]),
		-(vrt[Z] + g_objOffset[2]));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::clampAngle (double& angle) {
	while (angle < 0)
		angle += 360.0;
	while (angle > 360.0)
		angle -= 360.0;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::mouseReleaseEvent (QMouseEvent* ev) {
	const bool wasLeft = (m_lastButtons & Qt::LeftButton) && !(ev->buttons() & Qt::LeftButton);
	const bool wasRight = (m_lastButtons & Qt::RightButton) && !(ev->buttons() & Qt::RightButton);
	
	if (wasLeft) {
		if (m_planeDraw) {
			// If we have 4 verts, stop drawing.
			if (m_planeDrawVerts.size () >= 4) {
				endPlaneDraw (true);
				return;
			}
			
			// If we picked an already-existing vertex, stop drawing
			for (vertex& vert : m_planeDrawVerts) {
				if (vert == m_hoverpos) {
					endPlaneDraw (true);
					return;
				}
			}
			
			m_planeDrawVerts.push_back (m_hoverpos);
			
			update ();
			return;
		} else {
			if (!m_rangepick)
				m_addpick = (m_keymods & Qt::ControlModifier);
			
			if (m_totalmove < 10 || m_rangepick)
				pick (ev->x (), ev->y ());
		}
		
		m_rangepick = false;
		m_totalmove = 0;
		return;
	}
	
	if (wasRight && m_planeDraw) {
		if (m_planeDrawVerts.size () > 0) {
			// Remove the last vertex
			m_planeDrawVerts.erase (m_planeDrawVerts.end () - 1);
		} else {
			endPlaneDraw (false);
			return;
		}
		
		update ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::mousePressEvent (QMouseEvent* ev) {
	if (ev->buttons () & Qt::LeftButton)
		m_totalmove = 0;
	
	if (ev->modifiers () & Qt::ShiftModifier) {
		m_rangepick = true;
		m_rangeStart.setX (ev->x ());
		m_rangeStart.setY (ev->y ());
		
		m_addpick = (m_keymods & Qt::ControlModifier);
	}
	
	m_lastButtons = ev->buttons ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::mouseMoveEvent (QMouseEvent* ev) {
	int dx = ev->x () - m_pos.x ();
	int dy = ev->y () - m_pos.y ();
	m_totalmove += abs (dx) + abs (dy);
	
	if (ev->buttons () & Qt::LeftButton && !m_rangepick) {
		m_rotX = m_rotX + (dy);
		m_rotY = m_rotY + (dx);
		
		clampAngle (m_rotX);
		clampAngle (m_rotY);
	}
	
	if (ev->buttons () & Qt::MidButton) {
		m_panX += 0.03f * dx * (m_zoom / 7.5f);
		m_panY -= 0.03f * dy * (m_zoom / 7.5f);
	}
	
	// Start the tool tip timer
	if (!m_drawToolTip)
		m_toolTipTimer->start (1000);
	
	m_pos = ev->pos ();
	update ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::keyPressEvent (QKeyEvent* ev) {
	m_keymods = ev->modifiers ();
}

void GLRenderer::keyReleaseEvent (QKeyEvent* ev) {
	m_keymods = ev->modifiers ();
}

// =============================================================================
void GLRenderer::wheelEvent (QWheelEvent* ev) {
	m_zoom *= (ev->delta () < 0) ? 1.2f : (1.0f / 1.2f);
	m_zoom = clamp (m_zoom, 0.01, 10000.0);
	
	update ();
	ev->accept ();
}

// =============================================================================
void GLRenderer::leaveEvent (QEvent* ev) {
	Q_UNUSED (ev);
	m_drawToolTip = false;
	m_toolTipTimer->stop ();
	update ();
}

// =============================================================================
void GLRenderer::contextMenuEvent (QContextMenuEvent* ev) {
	g_win->spawnContextMenu (ev->globalPos ());
}

// =============================================================================
void GLRenderer::setCamera (const GLRenderer::Camera cam) {
	m_camera = cam;
	gl_camera = (int) cam;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::pick (uint mouseX, uint mouseY) {
	// Check if we selected a camera icon
	if (!m_rangepick) {
		QPoint pos (mouseX, mouseY);
		
		for (CameraIcon& info : g_CameraIcons) {
			if (info.destRect.contains (pos)) {
				setCamera (info.cam);
				update ();
				return;
			}
		}
	}
	
	GLint viewport[4];
	
	// Clear the selection if we do not wish to add to it.
	if (!m_addpick) {
		std::vector<LDObject*> oldsel = g_win->sel ();
		g_win->sel ().clear ();
		
		for (LDObject* obj : oldsel) {
			obj->setSelected (false);
			compileObject (obj);
		}
	}
	
	m_picking = true;
	
	// Paint the picking scene
	glDisable (GL_DITHER);
	glClearColor (1.0f, 1.0f, 1.0f, 1.0f);
	
	drawGLScene ();
	
	glGetIntegerv (GL_VIEWPORT, viewport);
	
	short x0 = mouseX,
		y0 = mouseY;
	short x1, y1;
	
	// Determine how big an area to read - with range picking, we pick by
	// the area given, with single pixel picking, we use an 1 x 1 area.
	if (m_rangepick) {
		x1 = m_rangeStart.x ();
		y1 = m_rangeStart.y ();
	} else {
		x1 = x0 + 1;
		y1 = y0 + 1;
	}
	
	// x0 and y0 must be less than x1 and y1, respectively.
	if (x0 > x1)
		dataswap (x0, x1);
	
	if (y0 > y1)
		dataswap (y0, y1);
	
	// Clamp the values to ensure they're within bounds
	x0 = max<short> (0, x0);
	y0 = max<short> (0, y0);
	x1 = min<short> (x1, m_width);
	y1 = min<short> (y1, m_height);
	
	const short areawidth = (x1 - x0);
	const short areaheight = (y1 - y0);
	const long numpixels = areawidth * areaheight;
	
	// Allocate space for the pixel data.
	uchar* const pixeldata = new uchar[4 * numpixels];
	uchar* pixelptr = &pixeldata[0];
	
	assert (viewport[3] == m_height);
	
	// Read pixels from the color buffer.
	glReadPixels (x0, viewport[3] - y1, areawidth, areaheight, GL_RGBA, GL_UNSIGNED_BYTE, pixeldata);
	
	LDObject* removedObj = null;
	
	// Go through each pixel read and add them to the selection.
	for (long i = 0; i < numpixels; ++i) {
		uint32 idx =
			(*(pixelptr) * 0x10000) +
			(*(pixelptr + 1) * 0x00100) +
			(*(pixelptr + 2) * 0x00001);
		pixelptr += 4;
		
		if (idx == 0xFFFFFF)
			continue; // White is background; skip
		
		LDObject* obj = g_curfile->object (idx);
		
		// If this is an additive single pick and the object is currently selected,
		// we remove it from selection instead.
		if (!m_rangepick && m_addpick) {
			bool removed = false;
			
			for (ulong i = 0; i < g_win->sel ().size(); ++i) {
				if (g_win->sel ()[i] == obj) {
					g_win->sel ().erase (g_win->sel ().begin () + i);
					obj->setSelected (false);
					removed = true;
					removedObj = obj;
				}
			}
			
			if (removed)
				break;
		}
		
		g_win->sel ().push_back (obj);
	}
	
	delete[] pixeldata;
	
	// Remove duplicate entries. For this to be effective, the vector must be
	// sorted first.
	std::vector<LDObject*>& sel = g_win->sel ();
	std::sort (sel.begin(), sel.end ());
	std::vector<LDObject*>::iterator pos = std::unique (sel.begin (), sel.end ());
	sel.resize (std::distance (sel.begin (), pos));
	
	// Update everything now.
	g_win->updateSelection ();
	
	// Recompile the objects now to update their color
	for (LDObject* obj : sel)
		compileObject (obj);
	
	if (removedObj)
		compileObject (removedObj);
	
	m_picking = false;
	m_rangepick = false;
	glEnable (GL_DITHER);
	
	setBackground ();
	update ();
}

// =============================================================================
void GLRenderer::beginPlaneDraw () {
	if (m_camera == Free)
		return; // Cannot draw with the free camera
	
	m_planeDraw = true;
	
	// Disable the context menu - we need the right mouse button
	// for removing vertices.
	setContextMenuPolicy (Qt::NoContextMenu);
	
	// Use the crosshair cursor when plane drawing.
	setCursor (Qt::CrossCursor);
	
	// Clear the selection when beginning to draw onto a plane.
	// FIXME: make the selection clearing stuff in ::pick a method and use it
	// here! This code doesn't update the GL lists.
	g_win->sel ().clear ();
	g_win->updateSelection ();
	update ();
}

// =============================================================================
void GLRenderer::endPlaneDraw (bool accept) {
	// If we accepted, clean the selection and create the object
	if (accept) {
		vector<vertex>& verts = m_planeDrawVerts;
		LDObject* obj = null;
		
		switch (verts.size ()) {
		case 1:
			{
				// 1 vertex - add a vertex object
				obj = new LDVertex;
				static_cast<LDVertex*> (obj)->pos = verts[0];
				obj->color = maincolor;
			}
			break;
		
		case 2:
			{
				// 2 verts - make a line
				obj = new LDLine;
				obj->color = edgecolor;
				for (ushort i = 0; i < 2; ++i)
					obj->coords[i] = verts[i];
			}
			break;
			
		case 3:
		case 4:
			{
				obj = (verts.size () == 3) ?
					static_cast<LDObject*> (new LDTriangle) :
					static_cast<LDObject*> (new LDQuad);
				
				obj->color = maincolor;
				for (ushort i = 0; i < obj->vertices (); ++i)
					obj->coords[i] = verts[i];
			}
			break;
		}
		
		if (obj) {
			g_curfile->addObject (obj);
			compileObject (obj);
			g_win->refresh ();
			
			History::addEntry (new AddHistory ({(ulong) obj->getIndex (g_curfile)}, {obj->clone ()}));
		}
	}
	
	m_planeDraw = false;
	m_planeDrawVerts.clear ();
	
	unsetCursor ();
	
	// Restore the context menu
	setContextMenuPolicy (Qt::DefaultContextMenu);
	
	update ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::compileObject (LDObject* obj) {
	for (const GL::ListType listType : g_glListTypes) {
		GLuint list = glGenLists (1);
		glNewList (list, GL_COMPILE);
		
		obj->glLists[listType] = list;
		compileList (obj, listType);
		
		glEndList ();
	}
}

// =============================================================================
uchar* GLRenderer::screencap (ushort& w, ushort& h) {
	w = m_width;
	h = m_height;
	uchar* cap = new uchar[4 * w * h];
	
	m_screencap = true;
	update ();
	m_screencap = false;
	
	// Capture the pixels
	glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, cap);
	
	// Restore the background
	setBackground ();
	
	return cap;
}

// =============================================================================
void GLRenderer::slot_toolTipTimer () {
	// We come here if the cursor has stayed in one place for longer than a
	// a second. Check if we're holding it over a camera icon - if so, draw
	// a tooltip.
	for (CameraIcon& icon : g_CameraIcons) {
		if (icon.destRect.contains (m_pos)) {
			m_toolTipCamera = icon.cam;
			m_drawToolTip = true;
			update ();
			break;
		}
	}
}
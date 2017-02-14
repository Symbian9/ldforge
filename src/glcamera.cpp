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

#include "glcamera.h"
#include "grid.h"
#include "miscallenous.h"

/*
 * Constructs a fixed camera from parameters.
 */
GLCamera::GLCamera(QString name, FixedCameraParameters&& bag) :
    m_name {name},
    m_glrotate {bag.glRotateX, bag.glRotateY, bag.glRotateZ},
    m_localX {bag.localX},
    m_localY {bag.localY},
    m_negatedX {bag.negatedX},
    m_negatedY {bag.negatedY},
    m_negatedDepth {bag.negatedZ} {}

/*
 * Constructs a free camera.
 */
GLCamera::GLCamera(QString name, FreeToken) :
    m_name {name},
    m_isFree {true} {}

/*
 * Returns OpenGL rotation information for this camera.
 */
int GLCamera::glRotate(Axis axis) const
{
	return m_glrotate[axis];
}

/*
 * Returns whether or not the given axis is negated on this camera.
 */
bool GLCamera::isAxisNegated(Axis axis) const
{
	switch (axis)
	{
	case X:
		return m_negatedX;

	case Y:
		return m_negatedY;

	case Z:
		return m_negatedDepth;

	default:
		return false;
	}
}

/*
 * Returns the 3D axis that is on the X axis in this camera.
 */
Axis GLCamera::axisX() const
{
	return m_localX;
}

/*
 * Returns the 3D axis that is on the Y axis in this camera.
 */
Axis GLCamera::axisY() const
{
	return m_localY;
}

/*
 * Returns the 3D axis that is on the Z axis in this camera (inwards).
 */
Axis GLCamera::axisZ() const
{
	return static_cast<Axis>(3 - m_localX - m_localY);
}

/*
 * This converts a 2D point on the screen to a 3D point in the model. If 'snap' is true, the 3D point will snap to the current grid.
 */
Vertex GLCamera::convert2dTo3d(const QPoint& position2d, Grid* grid) const
{
	if (m_isFree)
	{
		return {0, 0, 0};
	}
	else
	{
		Vertex position3d;
		int signX = m_negatedX ? -1 : 1;
		int signY = m_negatedY ? -1 : 1;

		// Calculate cx and cy - these are the LDraw unit coords the cursor is at.
		double cx = -m_virtualSize.width() + (2 * position2d.x() * m_virtualSize.width() / m_size.width()) - m_panningX;
		double cy = m_virtualSize.height() - (2 * position2d.y() * m_virtualSize.height() / m_size.height()) - m_panningY;

		// If a grid was passed, snap coordinates to it.
		if (grid)
		{
			cx = grid->snap(cx, Grid::Coordinate);
			cy = grid->snap(cy, Grid::Coordinate);
		}

		roundToDecimals(cx, 4);
		roundToDecimals(cy, 4);

		// Create the vertex from the coordinates
		position3d.setCoordinate(axisX(), cx * signX);
		position3d.setCoordinate(axisY(), cy * signY);
		position3d.setCoordinate(axisZ(), m_depth);
		return position3d;
	}
}

/*
 * Inverse operation for the above - convert a 3D position to a 2D screen position.
 */
QPoint GLCamera::convert3dTo2d(const Vertex& position3d) const
{
	if (m_isFree)
	{
		return {0, 0};
	}
	else
	{
		int signX = m_negatedX ? -1 : 1;
		int signY = m_negatedY ? -1 : 1;
		int rx = (position3d[axisX()] * signX + m_virtualSize.width() + m_panningX) * m_size.width() / 2 / m_virtualSize.width();
		int ry = (position3d[axisY()] * signY - m_virtualSize.height() + m_panningY) * m_size.height() / 2 / m_virtualSize.height();
		return {rx, -ry};
	}
}

/*
 * Resizes the camera when the renderer is resized.
 */
void GLCamera::rendererResized(int width, int height)
{
	m_size = {width, height};
	m_virtualSize = {m_zoom, height * m_zoom / width};
}

/*
 * Returns the "virtual size" of the camera. Used to zoom in while keeping proportions.
 */
const QSizeF& GLCamera::virtualSize() const
{
	return m_virtualSize;
}

/*
 * Returns the "z depth" of the camera. Since the camera provides 2D editing, this value fills in the value for the
 * third dimension for 3D vertices.
 */
double GLCamera::depth() const
{
	return m_depth;
}

/*
 * Returns the X-panning of this camera.
 */
double GLCamera::panningX() const
{
	return m_panningX;
}

/*
 * Returns the Y-panning of this camera.
 */
double GLCamera::panningY() const
{
	return m_panningY;
}

/*
 * Returns the zoom level of this camera.
 */
double GLCamera::zoom() const
{
	return m_zoom;
}

/*
 * Explicitly sets the panning of this camera.
 */
void GLCamera::setPanning(double x, double y)
{
	m_panningX = x;
	m_panningY = y;
}

/*
 * Makes the camera pan by the provided mouse move input.
 */
void GLCamera::pan(int xMove, int yMove)
{
	m_panningX += 0.03f * xMove * zoom() / 7.5f;
	m_panningY -= 0.03f * yMove * zoom() / 7.5f;
}

/*
 * Zooms the camera in one notch (e.g. by mousewheel).
 */
void GLCamera::zoomNotch (bool inward)
{
	m_zoom *= inward ? 0.833f : 1.2f;
	m_zoom = qBound(0.01, zoom(), 10000.0);
	rendererResized(m_size.width(), m_size.height());
}

/*
 * Explicitly sets the zoom of this camera.
 */
void GLCamera::setZoom(double zoom)
{
	m_zoom = zoom;
	rendererResized(m_size.width(), m_size.height());
}

/*
 * Returns the name of the camera
 */
const QString& GLCamera::name() const
{
	return m_name;
}

/*
 * Returns the camera's transformation matrix, scaled by the given scale value.
 */
Matrix GLCamera::transformationMatrix(double scale) const
{
	// Matrix templates. 2 is substituted with the scale value, 1 is inverted to -1 if needed.
	static const Matrix templates[3] =
	{
	    { 2, 0, 0, 0, 1, 0, 0, 0, 2 },
	    { 2, 0, 0, 0, 0, 2, 0, 1, 0 },
	    { 0, 1, 0, 2, 0, 0, 0, 0, 2 },
	};

	Matrix transform;

	if (m_localX == X and m_localY == Z)
		transform = templates[0];
	else if (m_localX == X and m_localY == Y)
		transform = templates[1];
	if (m_localX == Y and m_localY == Z)
		transform = templates[2];

	for (double& value : transform)
	{
		if (value == 2)
			value = scale;
		else if (value == 1 and (glRotate(X) + glRotate(Y) + glRotate(Z)) < 0)
			value = -1;
	}

	return transform;
}

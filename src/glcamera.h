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
#include "main.h"

struct FixedCameraParameters
{
	int glRotateX;
	int glRotateY;
	int glRotateZ;
	Axis localX;
	Axis localY;
	bool negatedX;
	bool negatedY;
	bool negatedZ;
};

/*
 * Models a 2D x/y co-ordinate system that maps to a fixed camera position.
 * Owns camera orientation information and provides 2D←→3D translation.
 */
class GLCamera : public QObject
{
	Q_OBJECT

public:
	// This is used to construct the free camera
	enum FreeToken { FreeCamera };

	GLCamera(QString name, FixedCameraParameters&& bag);
	GLCamera(QString name, FreeToken);

	Axis axisX() const;
	Axis axisY() const;
	Axis axisZ() const;
	Vertex convert2dTo3d(const QPoint& pos2d, Grid* grid = nullptr) const;
	QPoint convert3dTo2d(const Vertex& pos3d) const;
	double depth() const;
	int glRotate(Axis axis) const;
	bool isAxisNegated(Axis axis) const;
	const QString& name() const;
	void pan(int xMove, int yMove);
	double panningX() const;
	double panningY() const;
	Q_SLOT void rendererResized(int width, int height);
	void setPanning(double x, double y);
	void setZoom(double zoom);
	Matrix transformationMatrix(double scale) const;
	const QSizeF& virtualSize() const;
	double zoom() const;
	void zoomNotch(bool inward);

private:
	QString m_name;
	double m_panningX = 0;
	double m_panningY = 0;
	double m_depth = 0;
	double m_zoom = 30;
	QSize m_size;
	QSizeF m_virtualSize;
	int m_glrotate[3] = {0, 0, 0}; // GL model transformation to use
	Axis m_localX = X; // Which axis to use for Y
	Axis m_localY = Y; // Which axis to use for Y
	bool m_isFree = false; // Is this the free camera?
	bool m_negatedX = false; // Is +x to the left?
	bool m_negatedY = false; // Is +y downwards?
	bool m_negatedDepth = false; // is greater depth value closer to camera?
};

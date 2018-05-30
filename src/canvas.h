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
#include "glrenderer.h"
#include "editmodes/abstractEditMode.h"
#include "geometry/plane.h"

class Canvas : public GLRenderer
{
public:
	Canvas(LDDocument* document, QWidget* parent = nullptr);
	~Canvas();

	EditModeType currentEditModeType() const;
	const Plane& drawPlane() const;
	int depthNegateFactor() const;
	LDDocument* document() const;
	void drawPoint(QPainter& painter, QPointF pos, QColor color = QColor (64, 192, 0)) const;
	void drawBlipCoordinates(QPainter& painter, const Vertex& pos3d) const;
	void drawBlipCoordinates(QPainter& painter, const Vertex& pos3d, QPointF pos) const;
	void clearCurrentCullValue();
	double currentCullValue() const;
	void getRelativeAxes(Axis& relX, Axis& relY) const;
	Axis getRelativeZ() const;
	QPen linePen() const;
	const Vertex& position3D() const;
	void setDrawPlane(const Plane& plane);
	void setCullValue(double value);
	void setEditMode(EditModeType type);

protected:
	void contextMenuEvent(QContextMenuEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void drawFixedCameraBackdrop() override;
	void dropEvent(QDropEvent* event) override;
	bool freeCameraAllowed() const override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void overpaint(QPainter& painter) override;

private:
	LDDocument& m_document;
	AbstractEditMode* m_currentEditMode = nullptr;
	Vertex m_position3D;
	Plane m_drawPlane;
	double cullValues[6] = {0};
};

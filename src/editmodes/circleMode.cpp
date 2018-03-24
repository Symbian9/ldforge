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

#include <QPainter>
#include "circleMode.h"
#include "../lddocument.h"
#include "../ringFinder.h"
#include "../primitives.h"
#include "../canvas.h"
#include "../mainwindow.h"
#include "../documentmanager.h"
#include "../grid.h"
#include "../linetypes/modelobject.h"
#include "../linetypes/quadrilateral.h"
#include "../algorithms/geometry.h"

CircleMode::CircleMode(Canvas* canvas) :
    Super {canvas} {}


EditModeType CircleMode::type() const
{
	return EditModeType::Circle;
}


double CircleMode::getCircleDrawDist(int position) const
{
	if (countof(m_drawedVerts) >= position + 1)
	{
		Vertex v1;

		if (countof(m_drawedVerts) >= position + 2)
			v1 = m_drawedVerts[position + 1];
		else
			v1 = renderer()->currentCamera().convert2dTo3d(renderer()->mousePosition(), grid());

		Axis localx, localy;
		renderer()->getRelativeAxes(localx, localy);
		double dx = m_drawedVerts[0][localx] - v1[localx];
		double dy = m_drawedVerts[0][localy] - v1[localy];
		return grid()->snap({hypot(dx, dy), 0}).x();
	}

	return 0.0;
}


void CircleMode::endDraw()
{
	Model model {m_documents};
	PrimitiveModel primitiveModel;
	primitiveModel.segments = m_window->ringToolSegments();
	primitiveModel.divisions = m_window->ringToolDivisions();
	primitiveModel.ringNumber = 0;
	double dist0 (getCircleDrawDist (0));
	double dist1 (getCircleDrawDist (1));
	LDDocument* primitiveFile;
	Matrix transform;
	bool circleOrDisc = false;

	if (dist1 < dist0)
		qSwap(dist0, dist1);

	if (dist0 == dist1)
	{
		// If the radii are the same, there's no ring space to fill. Use a circle.
		primitiveModel.type = PrimitiveModel::Circle;
		primitiveFile = primitives()->getPrimitive(primitiveModel);
		transform = Matrix::fromRotationMatrix(renderer()->currentCamera().transformationMatrix(dist0));
		circleOrDisc = true;
	}
	else if (dist0 == 0 or dist1 == 0)
	{
		// If either radii is 0, use a disc.
		primitiveModel.type = PrimitiveModel::Disc;
		primitiveFile = primitives()->getPrimitive(primitiveModel);
		transform = Matrix::fromRotationMatrix(renderer()->currentCamera().transformationMatrix((dist0 != 0) ? dist0 : dist1));
		circleOrDisc = true;
	}
	else if (g_RingFinder.findRings(dist0, dist1))
	{
		// The ring finder found a solution, use that. Add the component rings to the file.
		primitiveModel.type = PrimitiveModel::Ring;

		for (const RingFinder::Component& component : g_RingFinder.bestSolution()->getComponents())
		{
			primitiveModel.ringNumber = component.num;
			primitiveFile = primitives()->getPrimitive(primitiveModel);
			Matrix matrix = Matrix::fromRotationMatrix(renderer()->currentCamera().transformationMatrix(component.scale));
			model.emplace<LDSubfileReference>(primitiveFile->name(), matrix, m_drawedVerts.first());
		}
	}
	else
	{
		// Ring finder failed, last resort: draw the ring with quads
		Axis localx, localy, localz;
		renderer()->getRelativeAxes (localx, localy);
		localz = (Axis) (3 - localx - localy);
		double x0 = m_drawedVerts[0][localx];
		double y0 = m_drawedVerts[0][localy];

		Vertex templ;
		templ.setCoordinate(localx, x0);
		templ.setCoordinate(localy, y0);
		templ.setCoordinate(localz, renderer()->getDepthValue());

		// Calculate circle coords
		QVector<QLineF> c0 = makeCircle(primitiveModel.segments, primitiveModel.divisions, dist0);
		QVector<QLineF> c1 = makeCircle(primitiveModel.segments, primitiveModel.divisions, dist1);

		for (int i = 0; i < primitiveModel.segments; ++i)
		{
			Vertex v0, v1, v2, v3;
			v0 = v1 = v2 = v3 = templ;
			v0.setCoordinate (localx, v0[localx] + c0[i].x1());
			v0.setCoordinate (localy, v0[localy] + c0[i].y1());
			v1.setCoordinate (localx, v1[localx] + c0[i].x2());
			v1.setCoordinate (localy, v1[localy] + c0[i].y2());
			v2.setCoordinate (localx, v2[localx] + c1[i].x2());
			v2.setCoordinate (localy, v2[localy] + c1[i].y2());
			v3.setCoordinate (localx, v3[localx] + c1[i].x1());
			v3.setCoordinate (localy, v3[localy] + c1[i].y1());

			// Ensure the quads always are BFC-front towards the camera
			if (static_cast<int>(renderer()->camera()) % 3 <= 0)
				qSwap(v1, v3);

			LDQuadrilateral* quad = model.emplace<LDQuadrilateral>(v0, v1, v2, v3);
			quad->setColor(MainColor);
		}
	}

	if (circleOrDisc and primitiveFile)
		model.emplace<LDSubfileReference>(primitiveFile->name(), transform, m_drawedVerts.first());

	finishDraw (model);
}

/*
 * Which way around will we place our circle primitive? This only makes a difference if we're not drawing a full circle.
 * Result is an angle offset in radians.
 */
double CircleMode::orientation() const
{
	if (not m_drawedVerts.isEmpty())
	{
		int divisions = m_window->ringToolDivisions();
		QPointF originSpot = renderer()->currentCamera().convert3dTo2d(m_drawedVerts.first());
		// Line from the origin of the circle to current mouse position
		QLineF hand1 = {originSpot, renderer()->mousePositionF()};
		// Line from the origin spot to
		QLineF hand2 = {{0, 0}, {1, 0}};
		// Calculate the angle between these hands and round it to whole divisions.
		double angleoffset = roundToInterval(-hand1.angleTo(hand2), 360.0 / divisions);
		// Take the camera's depth coefficient into account here. This way, the preview is flipped if the
		// primitive also would be.
		return angleoffset * pi / 180.0 * renderer()->depthNegateFactor();
	}
	else
	{
		return 0.0;
	}
}


void CircleMode::render (QPainter& painter) const
{
	QFontMetrics metrics = QFontMetrics (QFont());

	// If we have not specified the center point of the circle yet, preview it on the screen.
	if (m_drawedVerts.isEmpty())
	{
		QPoint position2d = renderer()->currentCamera().convert3dTo2d(renderer()->position3D());
		renderer()->drawPoint(painter, position2d);
		renderer()->drawBlipCoordinates(painter, renderer()->position3D(), position2d);
		return;
	}

	QVector<Vertex> innerverts, outerverts;
	QVector<QPointF> innerverts2d, outerverts2d;
	double innerdistance = getCircleDrawDist(0);
	double outerdistance = countof(m_drawedVerts) >= 2 ? getCircleDrawDist (1) : -1;
	int divisions = m_window->ringToolDivisions();
	int segments = m_window->ringToolSegments();
	double angleUnit = 2 * pi / divisions;
	Axis relX, relY;
	renderer()->getRelativeAxes(relX, relY);
	double angleoffset = (countof(m_drawedVerts) < 3 ? orientation() : m_angleOffset);

	// Calculate the preview positions of vertices
	for (int i = 0; i < segments + 1; ++i)
	{
		const double sinangle = ldrawsin(angleoffset + i * angleUnit);
		const double cosangle = ldrawcos(angleoffset + i * angleUnit);
		Vertex vertex;
		vertex.setCoordinate (relX, m_drawedVerts[0][relX] + (cosangle * innerdistance));
		vertex.setCoordinate (relY, m_drawedVerts[0][relY] + (sinangle * innerdistance));
		innerverts << vertex;
		innerverts2d << renderer()->currentCamera().convert3dTo2d(vertex);

		if (outerdistance != -1)
		{
			vertex.setCoordinate (relX, m_drawedVerts[0][relX] + (cosangle * outerdistance));
			vertex.setCoordinate (relY, m_drawedVerts[0][relY] + (sinangle * outerdistance));
			outerverts << vertex;
			outerverts2d << renderer()->currentCamera().convert3dTo2d(vertex);
		}
	}

	QVector<QLineF> lines;

	if (outerdistance != -1 and outerdistance != innerdistance)
	{
		painter.setBrush(m_polybrush);
		painter.setPen(Qt::NoPen);
		lines.reserve(segments * 2);

		// Compile polygons
		for (int i = 0; i < segments; ++i)
		{
			QVector<QPointF> points;
			points << innerverts2d[i]
				<< innerverts2d[i + 1]
				<< outerverts2d[i + 1]
				<< outerverts2d[i];
			painter.drawPolygon (QPolygonF (points));
			lines.append({innerverts2d[i], innerverts2d[i + 1]});
			lines.append({outerverts2d[i], outerverts2d[i + 1]});
		}

		// Add bordering edges for unclosed rings/discs
		if (segments != divisions)
		{
			lines.append({innerverts2d.first(), outerverts2d.first()});
			lines.append({innerverts2d.last(), outerverts2d.last()});
		}
	}
	else
	{
		lines.reserve(segments);

		for (int i = 0; i < segments; ++i)
			lines.append({innerverts2d[i], innerverts2d[i + 1]});
	}

	// Draw green blips at where the points are
	for (const QPointF& point : innerverts2d + outerverts2d)
		renderer()->drawPoint(painter, point);

	// Draw edge lines
	painter.setPen(renderer()->linePen());
	painter.drawLines(lines);

	// Draw the current radius in the middle of the circle.
	QPoint origin = renderer()->currentCamera().convert3dTo2d (m_drawedVerts[0]);
	QString label = QString::number (innerdistance);
	painter.setPen(renderer()->textPen());
	painter.drawText(origin.x() - (metrics.width(label) / 2), origin.y(), label);

	if (countof(m_drawedVerts) >= 2)
	{
		painter.drawText(origin.x() - (metrics.width(label) / 2),
			origin.y() + metrics.height(), QString::number(outerdistance));
	}
}


bool CircleMode::preAddVertex (const Vertex&)
{
	m_angleOffset = orientation();
	return false;
}


int CircleMode::maxVertices() const
{
	return 3;
}

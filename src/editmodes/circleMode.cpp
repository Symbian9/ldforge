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
#include "../linetypes/circularprimitive.h"
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

#if 0
static Matrix shearMatrixForPlane(Canvas* renderer)
{
	const Plane& plane = renderer->drawPlane();
	Axis localx, localy;
	renderer->getRelativeAxes(localx, localy);
	Axis localz = renderer->getRelativeZ();
	Matrix shearMatrix = Matrix::identity;
	Vertex normalAsVertex = Vertex::fromVector(plane.normal);

	// Compute shear matrix values. The (Y, X) cell means a slope for Y in regard to X.
	// If x grows by 2, y grows by 2 times this value. In the circle primitives,
	// depth is Y, but in the orthogonal view, depth is Z. So Y and Z must be swapped.
	if (not qFuzzyCompare(normalAsVertex[localz], 0.0))
	{
		// The slope of the vector is 90° offset from the normal vector. So Y/X slope is
		// -X/Y from the normal vector.
		shearMatrix(Y, X) = -normalAsVertex[localx] / normalAsVertex[localz];
		shearMatrix(Y, Z) = -normalAsVertex[localy] / normalAsVertex[localz];
	}

	return shearMatrix;
}
#endif

void CircleMode::endDraw()
{
	Model model {m_documents};
	CircularSection section = m_window->circleToolSection();
	double dist0 = getCircleDrawDist(0);
	double dist1 = getCircleDrawDist(1);
	QVector3D translation = m_drawedVerts.first().toVector();

	if (dist1 < dist0)
		qSwap(dist0, dist1);

	if (qFuzzyCompare(dist0, dist1))
	{
		// Special case: radii are the same, there's no area. Use a circle.
		// transform = shearMatrixForPlane(renderer());
		QMatrix4x4 transform = renderer()->currentCamera().transformationMatrix(1);
		transform.scale(dist0);
		offset(transform, translation);
		model.emplace<LDCircularPrimitive>(PrimitiveModel::Circle, section.segments, section.divisions, transform);
		finishDraw(model);
		return;
	}
	else if (qFuzzyCompare(dist0, 0) or qFuzzyCompare(dist1, 0))
	{
		// Special case #2: one radius is 0, so use a disc.
		//transform = shearMatrixForPlane(renderer());
		QMatrix4x4 transform = renderer()->currentCamera().transformationMatrix(1);
		transform.scale(max(dist0, dist1));
		offset(transform, translation);
		model.emplace<LDCircularPrimitive>(PrimitiveModel::Disc, section.segments, section.divisions, transform);
		finishDraw(model);
		return;
	}
	else if (g_RingFinder.findRings(dist0, dist1)) // Consult the ring finder now
	{
		// The ring finder found a solution, use that. Add the component rings to the file.
		PrimitiveModel primitiveModel;
		CircularSection section = m_window->circleToolSection();
		primitiveModel.segments = section.segments;
		primitiveModel.divisions = section.divisions;
		primitiveModel.type = PrimitiveModel::Ring;

		for (const RingFinder::Component& component : g_RingFinder.bestSolution()->getComponents())
		{
			primitiveModel.ringNumber = component.num;
			LDDocument* primitiveFile = primitives()->getPrimitive(primitiveModel);
			QMatrix4x4 matrix = renderer()->currentCamera().transformationMatrix(component.scale);
			offset(matrix, translation);
			// matrix = shearMatrixForPlane(renderer()) * matrix;
			model.emplace<LDSubfileReference>(primitiveFile->name(), matrix);
		}
	}
	else
	{
		// Ring finder failed, last resort: draw the ring with quads
		Axis localx, localy;
		renderer()->getRelativeAxes (localx, localy);
		double x0 = m_drawedVerts[0][localx];
		double y0 = m_drawedVerts[0][localy];

		Vertex templ;
		templ.setCoordinate(localx, x0);
		templ.setCoordinate(localy, y0);

		// Calculate circle coords
		QVector<QLineF> c0 = makeCircle(section.segments, section.divisions, dist0);
		QVector<QLineF> c1 = makeCircle(section.segments, section.divisions, dist1);

		for (int i = 0; i < section.segments; ++i)
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

			// Project the vertices onto the draw plane.
			for (Vertex* vertex : {&v0, &v1, &v2, &v3})
				*vertex = projectToDrawPlane(*vertex);

			LDQuadrilateral* quad = model.emplace<LDQuadrilateral>(v0, v1, v2, v3);
			quad->setColor(MainColor);
		}
	}

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
		int divisions = m_window->circleToolSection().divisions;
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
	CircularSection section = m_window->circleToolSection();
	double angleUnit = 2 * pi / section.divisions;
	Axis relX, relY;
	renderer()->getRelativeAxes(relX, relY);
	double angleoffset = (countof(m_drawedVerts) < 3 ? orientation() : m_angleOffset);

	// Calculate the preview positions of vertices
	for (int i = 0; i < section.segments + 1; ++i)
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
		lines.reserve(section.segments * 2);

		// Compile polygons
		for (int i = 0; i < section.segments; ++i)
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
		if (section.segments != section.divisions)
		{
			lines.append({innerverts2d.first(), outerverts2d.first()});
			lines.append({innerverts2d.last(), outerverts2d.last()});
		}
	}
	else
	{
		lines.reserve(section.segments);

		for (int i = 0; i < section.segments; ++i)
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

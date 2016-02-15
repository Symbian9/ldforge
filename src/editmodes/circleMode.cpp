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

#include <QPainter>
#include "circleMode.h"
#include "../miscallenous.h"
#include "../ldObject.h"
#include "../ldDocument.h"
#include "../ringFinder.h"
#include "../primitives.h"
#include "../glRenderer.h"
#include "../mainwindow.h"
#include "../ldObjectMath.h"

CircleMode::CircleMode (GLRenderer* renderer) :
	Super (renderer) {}

EditModeType CircleMode::type() const
{
	return EditModeType::Circle;
}

double CircleMode::getCircleDrawDist (int pos) const
{
	if (m_drawedVerts.size() >= pos + 1)
	{
		Vertex v1 = (m_drawedVerts.size() >= pos + 2) ? m_drawedVerts[pos + 1] :
			renderer()->convert2dTo3d (renderer()->mousePosition(), false);
		Axis localx, localy;
		renderer()->getRelativeAxes (localx, localy);
		double dx = m_drawedVerts[0][localx] - v1[localx];
		double dy = m_drawedVerts[0][localy] - v1[localy];
		return snapToGrid (sqrt ((dx * dx) + (dy * dy)), Grid::Coordinate);
	}

	return 0.0;
}

Matrix CircleMode::getCircleDrawMatrix (double scale)
{
	// Matrix templates. 2 is substituted with the scale value, 1 is inverted to -1 if needed.
	static const Matrix templates[3] =
	{
		{ 2, 0, 0, 0, 1, 0, 0, 0, 2 },
		{ 2, 0, 0, 0, 0, 2, 0, 1, 0 },
		{ 0, 1, 0, 2, 0, 0, 0, 0, 2 },
	};

	Matrix transform = templates[renderer()->camera() % 3];

	for (int i = 0; i < 9; ++i)
	{
		if (transform[i] == 2)
			transform[i] = scale;
		else if (transform[i] == 1 and renderer()->camera() >= 3)
			transform[i] = -1;
	}

	return transform;
}

void CircleMode::buildCircle()
{
	LDObjectList objs;
	const int segments (m_window->ringToolSegments());
	const int divisions (m_window->ringToolHiRes() ? HighResolution : LowResolution);
	double dist0 (getCircleDrawDist (0));
	double dist1 (getCircleDrawDist (1));
	LDDocument* refFile;
	Matrix transform;
	bool circleOrDisc = false;

	if (dist1 < dist0)
		qSwap (dist0, dist1);

	if (dist0 == dist1)
	{
		// If the radii are the same, there's no ring space to fill. Use a circle.
		refFile = primitives()->getPrimitive(::Circle, segments, divisions, 0);
		transform = getCircleDrawMatrix (dist0);
		circleOrDisc = true;
	}
	else if (dist0 == 0 or dist1 == 0)
	{
		// If either radii is 0, use a disc.
		refFile = primitives()->getPrimitive(::Disc, segments, divisions, 0);
		transform = getCircleDrawMatrix ((dist0 != 0) ? dist0 : dist1);
		circleOrDisc = true;
	}
	else if (g_RingFinder.findRings (dist0, dist1))
	{
		// The ring finder found a solution, use that. Add the component rings to the file.
		for (const RingFinder::Component& cmp : g_RingFinder.bestSolution()->getComponents())
		{
			refFile = primitives()->getPrimitive(::Ring, segments, divisions, cmp.num);
			LDSubfileReference* ref = LDSpawn<LDSubfileReference>();
			ref->setFileInfo (refFile);
			ref->setTransform (getCircleDrawMatrix (cmp.scale));
			ref->setPosition (m_drawedVerts[0]);
			ref->setColor (MainColor);
			objs << ref;
		}
	}
	else
	{
		// Ring finder failed, last resort: draw the ring with quads
		QList<QLineF> c0, c1;
		Axis localx, localy, localz;
		renderer()->getRelativeAxes (localx, localy);
		localz = (Axis) (3 - localx - localy);
		double x0 (m_drawedVerts[0][localx]);
		double y0 (m_drawedVerts[0][localy]);

		Vertex templ;
		templ.setCoordinate (localx, x0);
		templ.setCoordinate (localy, y0);
		templ.setCoordinate (localz, renderer()->getDepthValue());

		// Calculate circle coords
		primitives()->makeCircle(segments, divisions, dist0, c0);
		primitives()->makeCircle(segments, divisions, dist1, c1);

		for (int i = 0; i < segments; ++i)
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

			LDQuad* quad (LDSpawn<LDQuad> (v0, v1, v2, v3));
			quad->setColor (MainColor);

			// Ensure the quads always are BFC-front towards the camera
			if (renderer()->camera() % 3 <= 0)
				quad->invert();

			objs << quad;
		}
	}

	if (circleOrDisc and refFile)
	{
		LDSubfileReference* ref = LDSpawn<LDSubfileReference>();
		ref->setFileInfo (refFile);
		ref->setTransform (transform);
		ref->setPosition (m_drawedVerts[0]);
		ref->setColor (MainColor);
		objs << ref;
	}

	if (not objs.isEmpty())
	{
		Axis relZ = renderer()->getRelativeZ();;
		const int l (relZ == X ? 1 : 0);
		const int m (relZ == Y ? 1 : 0);
		const int n (relZ == Z ? 1 : 0);
		RotateObjects (l, m, n, -m_angleOffset, objs);
	}

	finishDraw (objs);
}

double CircleMode::getAngleOffset() const
{
	if (m_drawedVerts.isEmpty())
		return 0.0;

	const int divisions (m_window->ringToolHiRes() ? HighResolution : LowResolution);
	QPointF originspot (renderer()->convert3dTo2d (m_drawedVerts.first()));
	QLineF bearing (originspot, renderer()->mousePositionF());
	QLineF bearing2 (originspot, QPointF (originspot.x(), 0.0));
	double angleoffset (-bearing.angleTo (bearing2) + 90);
	angleoffset /= (360.0 / divisions); // convert angle to 0-16 scale
	angleoffset = round (angleoffset); // round to nearest 16th
	angleoffset *= ((2 * Pi) / divisions); // convert to radians
	angleoffset *= renderer()->depthNegateFactor(); // negate based on camera
	return angleoffset;
}

void CircleMode::render (QPainter& painter) const
{
	QFontMetrics metrics = QFontMetrics (QFont());

	// If we have not specified the center point of the circle yet, preview it on the screen.
	if (m_drawedVerts.isEmpty())
	{
		QPoint pos2d = renderer()->convert3dTo2d (renderer()->position3D());
		renderer()->drawBlip (painter, pos2d);
		renderer()->drawBlipCoordinates (painter, renderer()->position3D(), pos2d);
		return;
	}

	QVector<Vertex> innerverts, outerverts;
	QVector<QPointF> innerverts2d, outerverts2d;
	const double innerdistance (getCircleDrawDist (0));
	const double outerdistance (m_drawedVerts.size() >= 2 ? getCircleDrawDist (1) : -1);
	const int divisions (m_window->ringToolHiRes() ? HighResolution : LowResolution);
	const int segments (m_window->ringToolSegments());
	const double angleUnit (2 * Pi / divisions);
	Axis relX, relY;
	renderer()->getRelativeAxes (relX, relY);
	const double angleoffset (m_drawedVerts.size() < 3 ? getAngleOffset() : m_angleOffset);

	// Calculate the preview positions of vertices
	for (int i = 0; i < segments + 1; ++i)
	{
		const double sinangle (sin (angleoffset + i * angleUnit));
		const double cosangle (cos (angleoffset + i * angleUnit));
		Vertex v (Origin);
		v.setCoordinate (relX, m_drawedVerts[0][relX] + (cosangle * innerdistance));
		v.setCoordinate (relY, m_drawedVerts[0][relY] + (sinangle * innerdistance));
		innerverts << v;
		innerverts2d << renderer()->convert3dTo2d (v);

		if (outerdistance != -1)
		{
			v.setCoordinate (relX, m_drawedVerts[0][relX] + (cosangle * outerdistance));
			v.setCoordinate (relY, m_drawedVerts[0][relY] + (sinangle * outerdistance));
			outerverts << v;
			outerverts2d << renderer()->convert3dTo2d (v);
		}
	}

	QVector<QLineF> lines (segments);

	if (outerdistance != -1 and outerdistance != innerdistance)
	{
		painter.setBrush (m_polybrush);
		painter.setPen (Qt::NoPen);

		// Compile polygons
		for (int i = 0; i < segments; ++i)
		{
			QVector<QPointF> points;
			points << innerverts2d[i]
				<< innerverts2d[i + 1]
				<< outerverts2d[i + 1]
				<< outerverts2d[i];
			painter.drawPolygon (QPolygonF (points));
			lines << QLineF (innerverts2d[i], innerverts2d[i + 1]);
			lines << QLineF (outerverts2d[i], outerverts2d[i + 1]);
		}

		// Add bordering edges for unclosed rings/discs
		if (segments != divisions)
		{
			lines << QLineF (innerverts2d.first(), outerverts2d.first());
			lines << QLineF (innerverts2d.last(), outerverts2d.last());
		}
	}
	else
	{
		for (int i = 0; i < segments; ++i)
			lines << QLineF (innerverts2d[i], innerverts2d[i + 1]);
	}

	// Draw green blips at where the points are
	for (QPointF const& point : innerverts2d + outerverts2d)
		renderer()->drawBlip (painter, point);

	// Draw edge lines
	painter.setPen (renderer()->linePen());
	painter.drawLines (lines);

	// Draw the current radius in the middle of the circle.
	QPoint origin = renderer()->convert3dTo2d (m_drawedVerts[0]);
	QString label = QString::number (innerdistance);
	painter.setPen (renderer()->textPen());
	painter.drawText (origin.x() - (metrics.width (label) / 2), origin.y(), label);

	if (m_drawedVerts.size() >= 2)
	{
		painter.drawText (origin.x() - (metrics.width (label) / 2),
			origin.y() + metrics.height(), QString::number (outerdistance));
	}
}

bool CircleMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	if (data.releasedButtons & Qt::LeftButton)
	{
		if (m_drawedVerts.size() < 3)
			addDrawnVertex (renderer()->position3D());
		else
			buildCircle();

		return true;
	}

	return false;
}

bool CircleMode::preAddVertex (const Vertex&)
{
	m_angleOffset = getAngleOffset();
	return false;
}

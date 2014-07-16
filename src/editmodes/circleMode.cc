/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
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

#include <QPainter>
#include "circleMode.h"
#include "../miscallenous.h"
#include "../ldObject.h"
#include "../ldDocument.h"
#include "../ringFinder.h"
#include "../primitives.h"
#include "../glRenderer.h"

CircleMode::CircleMode (GLRenderer* renderer) :
	Super (renderer) {}

EditModeType CircleMode::type() const
{
	return EditModeType::Circle;
}

double CircleMode::getCircleDrawDist (int pos) const
{
	assert (_drawedVerts.size() >= pos + 1);
	Vertex v1 = (_drawedVerts.size() >= pos + 2) ? _drawedVerts[pos + 1] :
		renderer()->coordconv2_3 (renderer()->mousePosition(), false);
	Axis localx, localy;
	renderer()->getRelativeAxes (localx, localy);
	double dx = _drawedVerts[0][localx] - v1[localx];
	double dy = _drawedVerts[0][localy] - v1[localy];
	return Grid::snap (sqrt ((dx * dx) + (dy * dy)), Grid::Coordinate);
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
		elif (transform[i] == 1 and renderer()->camera() >= 3)
			transform[i] = -1;
	}

	return transform;
}

void CircleMode::buildCircle()
{
	LDObjectList objs;
	const int segs = g_lores, divs = g_lores; // TODO: make customizable
	double dist0 = getCircleDrawDist (0),
		dist1 = getCircleDrawDist (1);
	LDDocumentPtr refFile;
	Matrix transform;
	bool circleOrDisc = false;

	if (dist1 < dist0)
		std::swap<double> (dist0, dist1);

	if (dist0 == dist1)
	{
		// If the radii are the same, there's no ring space to fill. Use a circle.
		refFile = getDocument ("4-4edge.dat");
		transform = getCircleDrawMatrix (dist0);
		circleOrDisc = true;
	}
	elif (dist0 == 0 or dist1 == 0)
	{
		// If either radii is 0, use a disc.
		refFile = getDocument ("4-4disc.dat");
		transform = getCircleDrawMatrix ((dist0 != 0) ? dist0 : dist1);
		circleOrDisc = true;
	}
	elif (g_RingFinder.findRings (dist0, dist1))
	{
		// The ring finder found a solution, use that. Add the component rings to the file.
		for (const RingFinder::Component& cmp : g_RingFinder.bestSolution()->getComponents())
		{
			// Get a ref file for this primitive. If we cannot find it in the
			// LDraw library, generate it.
			if ((refFile = ::getDocument (radialFileName (::Ring, g_lores, g_lores, cmp.num))) == null)
			{
				refFile = generatePrimitive (::Ring, g_lores, g_lores, cmp.num);
				refFile->setImplicit (false);
			}

			LDSubfilePtr ref = spawn<LDSubfile>();
			ref->setFileInfo (refFile);
			ref->setTransform (getCircleDrawMatrix (cmp.scale));
			ref->setPosition (_drawedVerts[0]);
			ref->setColor (maincolor());
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
		double x0 = _drawedVerts[0][localx],
			y0 = _drawedVerts[0][localy];

		Vertex templ;
		templ.setCoordinate (localx, x0);
		templ.setCoordinate (localy, y0);
		templ.setCoordinate (localz, renderer()->getDepthValue());

		// Calculate circle coords
		makeCircle (segs, divs, dist0, c0);
		makeCircle (segs, divs, dist1, c1);

		for (int i = 0; i < segs; ++i)
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

			LDQuadPtr quad (spawn<LDQuad> (v0, v1, v2, v3));
			quad->setColor (maincolor());

			// Ensure the quads always are BFC-front towards the camera
			if (renderer()->camera() % 3 <= 0)
				quad->invert();

			objs << quad;
		}
	}

	if (circleOrDisc and refFile != null)
	{
		LDSubfilePtr ref = spawn<LDSubfile>();
		ref->setFileInfo (refFile);
		ref->setTransform (transform);
		ref->setPosition (_drawedVerts[0]);
		ref->setColor (maincolor());
		objs << ref;
	}

	finishDraw (objs);
}

void CircleMode::render (QPainter& painter) const
{
	QFontMetrics metrics = QFontMetrics (QFont());

	// If we have not specified the center point of the circle yet, preview it on the screen.
	if (_drawedVerts.isEmpty())
	{
		renderer()->drawBlip (painter, renderer()->coordconv3_2 (renderer()->position3D()));
	}
	else
	{
		QVector<Vertex> verts, verts2;
		const double dist0 = getCircleDrawDist (0),
			dist1 = (_drawedVerts.size() >= 2) ? getCircleDrawDist (1) : -1;
		const int segs = g_lores;
		const double angleUnit = (2 * pi) / segs;
		Axis relX, relY;
		QVector<QPoint> ringpoints, circlepoints, circle2points;

		renderer()->getRelativeAxes (relX, relY);

		// Calculate the preview positions of vertices
		for (int i = 0; i < segs; ++i)
		{
			Vertex v = g_origin;
			v.setCoordinate (relX, _drawedVerts[0][relX] + (cos (i * angleUnit) * dist0));
			v.setCoordinate (relY, _drawedVerts[0][relY] + (sin (i * angleUnit) * dist0));
			verts << v;

			if (dist1 != -1)
			{
				v.setCoordinate (relX, _drawedVerts[0][relX] + (cos (i * angleUnit) * dist1));
				v.setCoordinate (relY, _drawedVerts[0][relY] + (sin (i * angleUnit) * dist1));
				verts2 << v;
			}
		}

		int i = 0;
		for (const Vertex& v : verts + verts2)
		{
			// Calculate the 2D point of the vertex
			QPoint point = renderer()->coordconv3_2 (v);

			// Draw a green blip at where it is
			renderer()->drawBlip (painter, point);

			// Add it to the list of points for the green ring fill.
			ringpoints << point;

			// Also add the circle points to separate lists
			if (i < verts.size())
				circlepoints << point;
			else
				circle2points << point;

			++i;
		}

		// Insert the first point as the seventeenth one so that
		// the ring polygon is closed properly.
		if (ringpoints.size() >= 16)
			ringpoints.insert (16, ringpoints[0]);

		// Same for the outer ring. Note that the indices are offset by 1
		// because of the insertion done above bumps the values.
		if (ringpoints.size() >= 33)
			ringpoints.insert (33, ringpoints[17]);

		// Draw the ring
		painter.setBrush ((_drawedVerts.size() >= 2) ? _polybrush : Qt::NoBrush);
		painter.setPen (Qt::NoPen);
		painter.drawPolygon (QPolygon (ringpoints));

		// Draw the circles
		painter.setBrush (Qt::NoBrush);
		painter.setPen (renderer()->linePen());
		painter.drawPolygon (QPolygon (circlepoints));
		painter.drawPolygon (QPolygon (circle2points));

		// Draw the current radius in the middle of the circle.
		QPoint origin = renderer()->coordconv3_2 (_drawedVerts[0]);
		QString label = QString::number (dist0);
		painter.setPen (renderer()->textPen());
		painter.drawText (origin.x() - (metrics.width (label) / 2), origin.y(), label);

		if (_drawedVerts.size() >= 2)
		{
			painter.drawText (origin.x() - (metrics.width (label) / 2),
				origin.y() + metrics.height(), QString::number (dist1));
		}
	}
}

bool CircleMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	if (data.releasedButtons & Qt::LeftButton)
	{
		if (_drawedVerts.size() < 3)
			addDrawnVertex (renderer()->position3D());
		else
			buildCircle();

		return true;
	}

	return false;
}

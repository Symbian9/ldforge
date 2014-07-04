#include <QPainter>
#include "circlemode.h"
#include "../miscallenous.h"
#include "../ldObject.h"

CircleMode::CircleMode (GLRenderer* renderer) :
	Super (renderer) {}

double CircleMode::getCircleDrawDist (int pos) const
{
	assert (m_drawedVerts.size() >= pos + 1);
	Vertex v1 = (m_drawedVerts.size() >= pos + 2) ? m_drawedVerts[pos + 1] :
		renderer()->coordconv2_3 (renderer()->mousePosition(), false);
	Axis localx, localy;
	renderer()->getRelativeAxes (localx, localy);
	double dx = m_drawedVerts[0][localx] - v1[localx];
	double dy = m_drawedVerts[0][localy] - v1[localy];
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
		elif (transform[i] == 1 && renderer()->camera() >= 3)
			transform[i] = -1;
	}

	return transform;
}

void CircleMode::render (QPainter& painter) const
{
	QFontMetrics const metrics (QFont());

	// If we have not specified the center point of the circle yet, preview it on the screen.
	if (m_drawedVerts.isEmpty())
	{
		renderer()->drawBlip (painter, renderer()->coordconv3_2 (renderer()->position3D()));
	}
	else
	{
		QVector<Vertex> verts, verts2;
		const double dist0 = getCircleDrawDist (0),
			dist1 = (m_drawedVerts.size() >= 2) ? getCircleDrawDist (1) : -1;
		const int segs = g_lores;
		const double angleUnit = (2 * pi) / segs;
		Axis relX, relY;
		QVector<QPoint> ringpoints, circlepoints, circle2points;

		renderer()->getRelativeAxes (relX, relY);

		// Calculate the preview positions of vertices
		for (int i = 0; i < segs; ++i)
		{
			Vertex v = g_origin;
			v.setCoordinate (relX, m_drawedVerts[0][relX] + (cos (i * angleUnit) * dist0));
			v.setCoordinate (relY, m_drawedVerts[0][relY] + (sin (i * angleUnit) * dist0));
			verts << v;

			if (dist1 != -1)
			{
				v.setCoordinate (relX, m_drawedVerts[0][relX] + (cos (i * angleUnit) * dist1));
				v.setCoordinate (relY, m_drawedVerts[0][relY] + (sin (i * angleUnit) * dist1));
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
		painter.setBrush ((m_drawedVerts.size() >= 2) ? _polybrush : Qt::NoBrush);
		painter.setPen (Qt::NoPen);
		painter.drawPolygon (QPolygon (ringpoints));

		// Draw the circles
		painter.setBrush (Qt::NoBrush);
		painter.setPen (renderer()->getLinePen());
		painter.drawPolygon (QPolygon (circlepoints));
		painter.drawPolygon (QPolygon (circle2points));

		// Draw the current radius in the middle of the circle.
		QPoint origin = renderer()->coordconv3_2 (m_drawedVerts[0]);
		QString label = QString::number (dist0);
		painter.setPen (renderer()->getTextPen());
		painter.drawText (origin.x() - (metrics.width (label) / 2), origin.y(), label);

		if (m_drawedVerts.size() >= 2)
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

	if (m_drawedVerts.size() < 3)
	{
		addDrawnVertex (m_position3D);
		return;
	}

	Super::mouseReleased (data);
}

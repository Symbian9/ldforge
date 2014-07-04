#include <QPainter>
#include "drawmode.h"

CFGENTRY (Bool,		drawLineLengths,			true)
CFGENTRY (Bool,		drawAngles,					false)

DrawMode::DrawMode (GLRenderer* renderer) :
	Super (renderer),
	_rectdraw (false) {}

EditModeType DrawMode::type() const
{
	return EditModeType::Draw;
}

void DrawMode::render (QPainter& painter) const
{
	QPoint poly[4];
	Vertex poly3d[4];
	int numverts = 4;
	QFontMetrics metrics (QFont());

	// Calculate polygon data
	if (not _rectdraw)
	{
		numverts = m_drawedVerts.size() + 1;
		int i = 0;

		for (Vertex& vert : m_drawedVerts)
			poly3d[i++] = vert;

		// Draw the cursor vertex as the last one in the list.
		if (numverts <= 4)
			poly3d[i] = renderer()->position3D();
		else
			numverts = 4;
	}
	else
	{
		// Get vertex information from m_rectverts
		if (m_drawedVerts.size() > 0)
			for (int i = 0; i < numverts; ++i)
				poly3d[i] = m_rectverts[i];
		else
			poly3d[0] = renderer()->position3D();
	}

	// Convert to 2D
	for (int i = 0; i < numverts; ++i)
		poly[i] = renderer()->coordconv3_2 (poly3d[i]);

	if (numverts > 0)
	{
		// Draw the polygon-to-be
		painter.setBrush (_polybrush);
		painter.drawPolygon (poly, numverts);

		// Draw vertex blips
		for (int i = 0; i < numverts; ++i)
		{
			QPoint& blip = poly[i];
			painter.setPen (renderer()->linePen());
			renderer()->drawBlip (painter, blip);

			// Draw their coordinates
			painter.setPen (renderer()->textPen());
			painter.drawText (blip.x(), blip.y() - 8, poly3d[i].toString (true));
		}

		// Draw line lenghts and angle info if appropriate
		if (numverts >= 2)
		{
			int numlines = (m_drawedVerts.size() == 1) ? 1 : m_drawedVerts.size() + 1;
			painter.setPen (renderer()->textPen());

			for (int i = 0; i < numlines; ++i)
			{
				const int j = (i + 1 < numverts) ? i + 1 : 0;
				const int h = (i - 1 >= 0) ? i - 1 : numverts - 1;

				if (cfg::drawLineLengths)
				{
					const QString label = QString::number ((poly3d[j] - poly3d[i]).length());
					QPoint origin = QLineF (poly[i], poly[j]).pointAt (0.5).toPoint();
					painter.drawText (origin, label);
				}

				if (cfg::drawAngles)
				{
					QLineF l0 (poly[h], poly[i]),
						l1 (poly[i], poly[j]);

					double angle = 180 - l0.angleTo (l1);

					if (angle < 0)
						angle = 180 - l1.angleTo (l0);

					QString label = QString::number (angle) + QString::fromUtf8 (QByteArray ("\302\260"));
					QPoint pos = poly[i];
					pos.setY (pos.y() + metrics.height());

					painter.drawText (pos, label);
				}
			}
		}
	}
}

bool DrawMode::preAddVertex (Vertex const& pos)
{
	// If we picked an already-existing vertex, stop drawing
	for (Vertex& vert : m_drawedVerts)
	{
		if (vert == pos)
		{
			endDraw (true);
			return true;
		}
	}

	return false;
}

void DrawMode::mouseReleased (MouseEventData const& data)
{
	if (_rectdraw)
	{
		if (m_drawedVerts.size() == 2)
		{
			endDraw (true);
			return;
		}
	}
	else
	{
		// If we have 4 verts, stop drawing.
		if (m_drawedVerts.size() >= 4)
		{
			endDraw (true);
			return;
		}

		if (m_drawedVerts.isEmpty() && ev->modifiers() & Qt::ShiftModifier)
		{
			_rectdraw = true;
			updateRectVerts();
		}
	}

	addDrawnVertex (renderer()->position3D());
}

#include <QPainter>
#include <QMouseEvent>
#include "drawmode.h"
#include "../ldObject.h"
#include "../glRenderer.h"

CFGENTRY (Bool, drawLineLengths, true)
CFGENTRY (Bool, drawAngles, false)

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
	QFontMetrics metrics = QFontMetrics (QFont());

	// Calculate polygon data
	if (not _rectdraw)
	{
		numverts = _drawedVerts.size() + 1;
		int i = 0;

		for (Vertex const& vert : _drawedVerts)
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
		if (_drawedVerts.size() > 0)
			for (int i = 0; i < numverts; ++i)
				poly3d[i] = _rectverts[i];
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
			int numlines = (_drawedVerts.size() == 1) ? 1 : _drawedVerts.size() + 1;
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
	for (Vertex& vert : _drawedVerts)
	{
		if (vert == pos)
		{
			endDraw();
			return true;
		}
	}

	return false;
}

bool DrawMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	if (data.releasedButtons & Qt::LeftButton)
	{
		if (_rectdraw)
		{
			if (_drawedVerts.size() == 2)
			{
				endDraw();
				return true;
			}
		}
		else
		{
			// If we have 4 verts, stop drawing.
			if (_drawedVerts.size() >= 4)
			{
				endDraw();
				return true;
			}

			if (_drawedVerts.isEmpty())
			{
				_rectdraw = (data.ev->modifiers() & Qt::ShiftModifier);
				updateRectVerts();
			}
		}

		addDrawnVertex (renderer()->position3D());
		return true;
	}

	return false;
}

//
// Update rect vertices when the mouse moves since the 3d position likely has changed
//
bool DrawMode::mouseMoved (QMouseEvent*)
{
	updateRectVerts();
	return false;
}

void DrawMode::updateRectVerts()
{
	if (not _rectdraw)
		return;

	if (_drawedVerts.isEmpty())
	{
		for (int i = 0; i < 4; ++i)
			_rectverts[i] = renderer()->position3D();

		return;
	}

	Vertex v0 = _drawedVerts[0],
		   v1 = (_drawedVerts.size() >= 2) ? _drawedVerts[1] : renderer()->position3D();

	const Axis localx = renderer()->getCameraAxis (false),
			   localy = renderer()->getCameraAxis (true),
			   localz = (Axis) (3 - localx - localy);

	for (int i = 0; i < 4; ++i)
		_rectverts[i].setCoordinate (localz, renderer()->getDepthValue());

	_rectverts[0].setCoordinate (localx, v0[localx]);
	_rectverts[0].setCoordinate (localy, v0[localy]);
	_rectverts[1].setCoordinate (localx, v1[localx]);
	_rectverts[1].setCoordinate (localy, v0[localy]);
	_rectverts[2].setCoordinate (localx, v1[localx]);
	_rectverts[2].setCoordinate (localy, v1[localy]);
	_rectverts[3].setCoordinate (localx, v0[localx]);
	_rectverts[3].setCoordinate (localy, v1[localy]);
}

void DrawMode::endDraw()
{
	// Clean the selection and create the object
	QList<Vertex>& verts = _drawedVerts;
	LDObjectList objs;

	if (_rectdraw)
	{
		LDQuadPtr quad (spawn<LDQuad>());

		updateRectVerts();

		for (int i = 0; i < quad->numVertices(); ++i)
			quad->setVertex (i, _rectverts[i]);

		quad->setColor (maincolor());
		objs << quad;
	}
	else
	{
		switch (verts.size())
		{
			case 1:
			{
				// 1 vertex - add a vertex object
				LDVertexPtr obj = spawn<LDVertex>();
				obj->pos = verts[0];
				obj->setColor (maincolor());
				objs << obj;
				break;
			}

			case 2:
			{
				// 2 verts - make a line
				LDLinePtr obj = spawn<LDLine> (verts[0], verts[1]);
				obj->setColor (edgecolor());
				objs << obj;
				break;
			}

			case 3:
			case 4:
			{
				LDObjectPtr obj = (verts.size() == 3) ?
					static_cast<LDObjectPtr> (spawn<LDTriangle>()) :
					static_cast<LDObjectPtr> (spawn<LDQuad>());

				obj->setColor (maincolor());

				for (int i = 0; i < verts.size(); ++i)
					obj->setVertex (i, verts[i]);

				objs << obj;
				break;
			}
		}
	}

	finishDraw (objs);
	_rectdraw = false;
}

#include "linePathMode.h"
#include "../glRenderer.h"
#include "../mainwindow.h"
#include <QKeyEvent>

LinePathMode::LinePathMode (GLRenderer *renderer) :
	Super (renderer) {}

void LinePathMode::render (QPainter& painter) const
{
	QVector<QPointF> points;
	QList<Vertex> points3d (m_drawedVerts);
	points3d << renderer()->position3D();

	for (Vertex const& vrt : points3d)
		points << renderer()->coordconv3_2 (vrt);

	painter.setPen (renderer()->textPen());

	if (points.size() == points3d.size())
	{
		for (int i = 0; i < points.size() - 1; ++i)
		{
			painter.drawLine (QLineF (points[i], points[i + 1]));
			drawLength (painter, points3d[i], points3d[i + 1], points[i], points[i + 1]);
		}
	
		for (QPointF const& point : points)
			renderer()->drawBlip (painter, point);
	}
}

bool LinePathMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	if (data.releasedButtons & Qt::LeftButton)
	{
		addDrawnVertex (renderer()->position3D());
		return true;
	}

	return false;
}

bool LinePathMode::preAddVertex (Vertex const& pos)
{
	// If we picked an the last vertex, stop drawing
	if (not m_drawedVerts.isEmpty() and pos == m_drawedVerts.last())
	{
		endDraw();
		return true;
	}

	return false;
}

void LinePathMode::endDraw()
{
	LDObjectList objs;

	for (int i = 0; i < m_drawedVerts.size() - 1; ++i)
	{
		LDLine* line = LDSpawn<LDLine>();
		line->setVertex (0, m_drawedVerts[i]);
		line->setVertex (1, m_drawedVerts[i + 1]);
		objs << line;
	}

	finishDraw (objs);
}

bool LinePathMode::keyReleased (QKeyEvent* ev)
{
	if (Super::keyReleased (ev))
		return true;

	if (ev->key() == Qt::Key_Enter or ev->key() == Qt::Key_Return)
	{
		endDraw();
		return true;
	}

	return false;
}

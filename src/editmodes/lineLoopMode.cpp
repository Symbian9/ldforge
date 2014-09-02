#include "lineLoopMode.h"
#include "../glRenderer.h"

LineLoopMode::LineLoopMode (GLRenderer *renderer) :
	Super (renderer) {}

void LineLoopMode::render (QPainter& painter) const
{
	renderer()->drawBlip (painter, renderer()->coordconv3_2 (renderer()->position3D()));
	QVector<QPointF> points;

	for (Vertex const& vrt : m_drawedVerts)
		points << renderer()->coordconv3_2 (vrt);

	painter.setPen (renderer()->textPen());

	for (int i = 0; i < points.size() - 1; ++i)
		painter.drawLine (QLineF (points[i], points[i + 1]));

	for (QPointF const& point : points)
		renderer()->drawBlip (painter, point);
}

bool LineLoopMode::mouseReleased (MouseEventData const& data)
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

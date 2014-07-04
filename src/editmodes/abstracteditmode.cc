#include <stdexcept>
#include "abstracteditmode.h"
#include "selectmode.h"
#include "drawmode.h"
#include "circlemode.h"
#include "magicwandmode.h"
#include "../mainWindow.h"

AbstractEditMode::AbstractEditMode (GLRenderer* renderer) :
	_renderer (renderer) {}

AbstractEditMode* AbstractEditMode::createByType (GLRenderer* renderer, EditModeType type)
{
	switch (type)
	{
		case EditModeType::Select: return new SelectMode (renderer);
		case EditModeType::Draw: return new DrawMode (renderer);
		case EditModeType::Circle: return new CircleMode (renderer);
		case EditModeType::MagicWand: return new MagicWandMode (renderer);
	}

	throw std::logic_error ("bad type given to AbstractEditMode::createByType");
}

GLRenderer* AbstractEditMode::renderer() const
{
	return _renderer;
}

AbstractDrawMode::AbstractDrawMode (GLRenderer* renderer) :
	AbstractEditMode (renderer),
	_polybrush (QBrush (QColor (64, 192, 0, 128)))
{
	// Disable the context menu - we need the right mouse button
	// for removing vertices.
	renderer->setContextMenuPolicy (Qt::NoContextMenu);

	// Use the crosshair cursor when drawing.
	renderer->setCursor (Qt::CrossCursor);

	// Clear the selection when beginning to draw.
	getCurrentDocument()->clearSelection();

	g_win->updateSelection();
	m_drawedVerts.clear();
}

AbstractSelectMode::AbstractSelectMode (GLRenderer* renderer) :
	AbstractEditMode (renderer)
{
	renderer->unsetCursor();
	renderer->setContextMenuPolicy (Qt::DefaultContextMenu);
}

// =============================================================================
//
void AbstractDrawMode::addDrawnVertex (Vertex const& pos)
{
	if (preAddVertex (pos))
		return;

	m_drawedVerts << pos;
}

virtual void AbstractDrawMode::mouseReleased (MouseEventData const& data)
{
	if (data.releasedButtons & Qt::MidButton)
	{
		// Find the closest vertex to our cursor
		double			minimumDistance = 1024.0;
		const Vertex*	closest = null;
		Vertex			cursorPosition = renderer()->coordconv2_3 (data.ev->pos(), false);
		QPoint			cursorPosition2D (data.ev->pos());
		const Axis		relZ = renderer()->getRelativeZ();
		QList<Vertex>	vertices;

		for (auto it = renderer()->document()->vertices().begin(); it != renderer()->document()->vertices().end(); ++it)
			vertices << it.key();

		// Sort the vertices in order of distance to camera
		std::sort (vertices.begin(), vertices.end(), [&](const Vertex& a, const Vertex& b) -> bool
		{
			if (renderer()->getFixedCamera (renderer()->camera()).negatedDepth)
				return a[relZ] > b[relZ];

			return a[relZ] < b[relZ];
		});

		for (const Vertex& vrt : vertices)
		{
			// If the vertex in 2d space is very close to the cursor then we use
			// it regardless of depth.
			QPoint vect2d = renderer()->coordconv3_2 (vrt) - cursorPosition2D;
			const double distance2DSquared = std::pow (vect2d.x(), 2) + std::pow (vect2d.y(), 2);
			if (distance2DSquared < 16.0 * 16.0)
			{
				closest = &vrt;
				break;
			}

			// Check if too far away from the cursor.
			if (distance2DSquared > 64.0 * 64.0)
				continue;

			// Not very close to the cursor. Compare using true distance,
			// including depth.
			const double distanceSquared = (vrt - cursorPosition).lengthSquared();

			if (distanceSquared < minimumDistance)
			{
				minimumDistance = distanceSquared;
				closest = &vrt;
			}
		}

		if (closest != null)
			addDrawnVertex (*closest);

		return true;
	}

	return false;
}

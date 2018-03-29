#include <QPainter>
#include <QPaintEvent>
#include "patterneditor.h"
#include "patternviewer.h"

/*
 * Construct a pattern viewer for an existing pattern editor.
 */
PatternViewer::PatternViewer(PatternEditor* parent) :
	QWidget {parent},
	editor {parent}
{
	this->transformation.scale(4, 4);
}

/*
 * Unfortunately, Qt doesn't provide an easy way to translate a floating point rectangle
 * into a floating point polygon. So here's a manual implementation.
 */
static QPolygonF transformRect(const QRectF& rect, const QTransform& transform)
{
	QPolygonF transformed;
	for (const QPointF& point : {
		rect.topLeft(),
		rect.topRight(),
		rect.bottomRight(),
		rect.bottomLeft()
	}) {
		transformed.append(transform.map(point));
	}
	return transformed;
}

/*
 * Renders the pattern.
 */
void PatternViewer::paintEvent(QPaintEvent* event)
{
	const Pattern& pattern = this->editor->pattern;
	static const QPixmap viewerBackground {":/data/pattern-background.png"};
	static const QPixmap canvasBackground {":/data/transparent-background.png"};
	QPainter painter {this};
	painter.drawTiledPixmap(this->rect(), viewerBackground);
	painter.setBrush(canvasBackground);
	QRectF canvasRect = QRectF {{0.0f, 0.0f}, pattern.canvasSize};
	painter.drawPolygon(::transformRect(canvasRect, this->transformation));
	painter.setTransform(this->transformation);
	event->accept();
}

#pragma once
#include <QPolygonF>
#include "../main.h"

struct ColoredPolygon
{
	QPolygonF geometry;
	LDColor color;

	ColoredPolygon(const QPolygonF& polygon, LDColor color);
};

struct Pattern
{
	Pattern(const QSizeF& size);

	QVector<ColoredPolygon> polygons;
	QSizeF canvasSize;
};

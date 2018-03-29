#include "pattern.h"

ColoredPolygon::ColoredPolygon(const QPolygonF& polygon, LDColor color) :
	geometry {polygon},
	color {color} {}

Pattern::Pattern(const QSizeF& size) :
	canvasSize {size}
{

}

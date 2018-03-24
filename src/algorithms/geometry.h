#pragma once
#include "../main.h"

QPointF pointOnCircumference(int segment, int divisions);
QVector<QLineF> makeCircle(int segments, int divisions, double radius);
qreal distanceFromPointToRectangle(const QPointF& point, const QRectF& rectangle);

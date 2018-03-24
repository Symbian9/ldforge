#pragma once
#include "../main.h"

double ldrawsin(double angle);
double ldrawcos(double angle);
QPointF pointOnLDrawCircumference(int segment, int divisions);
QVector<QLineF> makeCircle(int segments, int divisions, double radius);
qreal distanceFromPointToRectangle(const QPointF& point, const QRectF& rectangle);

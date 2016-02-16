/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <QVector>
#include <functional>
#include "main.h"
#include "basics.h"

class LDDocument;
class QColor;
class QAction;

using ApplyToMatrixFunction = std::function<void (int, double&)>;
using ApplyToMatrixConstFunction = std::function<void (int, double)>;

enum class RotationPoint
{
	ObjectOrigin,
	WorldOrigin,
	CustomPoint,
	NumValues
};

void applyToMatrix (Matrix& a, ApplyToMatrixFunction func);
void applyToMatrix (const Matrix& a, ApplyToMatrixConstFunction func);
void configureRotationPoint();
QString formatFileSize (qint64 size);
int gcd (int a, int b);
Vertex getRotationPoint (const LDObjectList& objs);
QString joinStrings (QList<StringFormatArg> vals, QString delim = " ");
void roundToDecimals (double& a, int decimals);
void simplify (int& numer, int& denom);

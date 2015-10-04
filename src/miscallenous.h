/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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
#include "configurationvaluebag.h"

class LDDocument;
class QColor;
class QAction;

// Simplifies the given fraction.
void Simplify (int& numer, int& denom);

using ApplyToMatrixFunction = std::function<void (int, double&)>;
using ApplyToMatrixConstFunction = std::function<void (int, double)>;

void RoundToDecimals (double& a, int decimals);
void ApplyToMatrix (Matrix& a, ApplyToMatrixFunction func);
void ApplyToMatrix (const Matrix& a, ApplyToMatrixConstFunction func);

double GetCoordinateOf (const Vertex& a, Axis ax);
QString MakePrettyFileSize (qint64 size);

QString Join (QList<StringFormatArg> vals, QString delim = " ");

// Grid stuff
float gridCoordinateSnap();
float gridAngleSnap();
float gridBezierCurveSegments();

// =============================================================================
enum class RotationPoint
{
	ObjectOrigin,
	WorldOrigin,
	CustomPoint,
	NumValues
};

Vertex GetRotationPoint (const LDObjectList& objs);
void ConfigureRotationPoint();

// =============================================================================
namespace Grid
{
	enum Type
	{
		Coarse,
		Medium,
		Fine
	};

	enum Config
	{
		Coordinate,
		Angle
	};

	double Snap (double value, const Grid::Config type);
}

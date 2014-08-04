/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
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
#include "configuration.h"
#include "main.h"
#include "basics.h"

#define NUM_PRIMES 500

class LDDocument;
class QColor;
class QAction;

// Prime numbers
extern const int PrimeNumbers[NUM_PRIMES];

// Simplifies the given fraction.
void simplify (int& numer, int& denom);

using ApplyToMatrixFunction = std::function<void (int, double&)>;
using ApplyToMatrixConstFunction = std::function<void (int, double)>;

void RoundToDecimals (double& a, int decimals);
void applyToMatrix (Matrix& a, ApplyToMatrixFunction func);
void applyToMatrix (const Matrix& a, ApplyToMatrixConstFunction func);

double GetCoordinateOf (const Vertex& a, Axis ax);
QString MakePrettyFileSize (qint64 size);

QString Join (QList<StringFormatArg> vals, QString delim = "");

// Grid stuff
struct GridData
{
	const char* name;
	float* coordinateSnap;
	float* angleSnap;
};

EXTERN_CFGENTRY (Int, Grid)
extern const GridData Grids[3];

inline const GridData& CurrentGrid()
{
	return Grids[cfg::Grid];
}

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

	double snap (double value, const Grid::Config type);
}

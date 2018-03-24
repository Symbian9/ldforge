/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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

#include "grid.h"
#include "configuration.h"

Grid::Grid(QObject* parent) :
	HierarchyElement(parent) {}


qreal Grid::coordinateSnap() const
{
	switch (config::grid())
	{
	default:
	case Grid::Coarse: return config::gridCoarseCoordinateSnap();
	case Grid::Medium: return config::gridMediumCoordinateSnap();
	case Grid::Fine: return config::gridFineCoordinateSnap();
	}
}


qreal Grid::angleSnap() const
{
	switch (config::grid())
	{
	default:
	case Grid::Coarse: return config::gridCoarseAngleSnap();
	case Grid::Medium: return config::gridMediumAngleSnap();
	case Grid::Fine: return config::gridFineAngleSnap();
	}
}


qreal Grid::angleAsRadians() const
{
	return (pi * angleSnap()) / 180;
}


int Grid::bezierCurveSegments() const
{
	switch (config::grid())
	{
	default:
	case Grid::Coarse: return config::gridCoarseBezierCurveSegments();
	case Grid::Medium: return config::gridMediumBezierCurveSegments();
	case Grid::Fine: return config::gridFineBezierCurveSegments();
	}
}


QPointF Grid::snap(QPointF point) const
{
	switch (type())
	{
	default:
	case Cartesian:
		{
			// For each co-ordinate, extract the amount of grid steps the value is away from zero, round that to remove the remainder,
			// and multiply back by the the grid size.
			double size = coordinateSnap();
			return {round(point.x() / size) * size, round(point.y() / size) * size};
		}

	case Polar:
		{
			qreal radius = hypot(point.x() - pole().x(), point.y() - -pole().y());
			qreal azimuth = atan2(point.y() - -pole().y(), point.x() - pole().x());
			double size = coordinateSnap();
			double angleStep = 2 * pi / polarDivisions();
			radius = round(radius / size) * size;
			azimuth = round(azimuth / angleStep) * angleStep;
			return {pole().x() + cos(azimuth) * radius, -pole().y() + sin(azimuth) * radius};
		}
	}
}

/*
 * Returns the pole of the grid, in ideal X/Y co-ordinates. Z is left up for the caller to decide.
 */
QPointF Grid::pole() const
{
	return {0, 0};
}

/*
 * Returns the amount of divisions (slices) to be used in the polar grid.
 */
int Grid::polarDivisions() const
{
	switch (config::grid())
	{
	default:
	case Coarse:
		return LowResolution;

	case Medium:
		return MediumResolution;

	case Fine:
		return HighResolution;
	}
}

/*
 * Returns whether to use a cartesian or polar grid.
 */
Grid::Type Grid::type() const
{
	return config::polarGrid() ? Polar : Cartesian;
}
